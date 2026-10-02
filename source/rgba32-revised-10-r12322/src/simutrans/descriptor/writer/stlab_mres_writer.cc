/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * MRES v2 writer (MRES-V2-SPEC.md). LAB CANDIDATE: `MRES` and `XR32` are
 * experimental node tags.
 *
 * Works on the compiled paks rather than on .dat input, so the 1x and 2x art go
 * through the ordinary compile path unchanged and this step only has to add the
 * pairing. The pak is handled as the tree it is on disk:
 *
 *   uint32 type | uint16 nchildren | uint16 size [| uint32 size if 0xFFFF]
 *   body, then the children depth first                  (obj_node.cc)
 *
 * Identity is the loader's (pakset_manager_t::multires_note_object): a top-level
 * object whose first child is its TEXT name, plus the child indices from that
 * object down to the IMG node. No image id is written anywhere.
 */

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <zlib.h>

#include "../../simtypes.h"
#include "stlab_mres_writer.h"


#define MRES_WRITE_VERSION (2)
#define MRES_MAX_DEPTH     (8)

namespace {

struct pnode_t {
	uint8  type[4];
	size_t start;                  ///< first header byte
	size_t body;                   ///< first body byte
	size_t body_len;
	size_t end;                    ///< one past the last byte of the last child
	std::vector<pnode_t> kids;
};


bool is_type(const pnode_t &n, const char *t)
{
	return memcmp(n.type, t, 4) == 0;
}


/// does this XR32 node (raw = its bytes, header included) end in a CRC-32 trailer?
bool xr32_has_crc(const pnode_t &n, const std::vector<uint8> &raw)
{
	const size_t body_at = n.body - n.start;
	return n.body_len >= 36  &&  (raw[body_at + 5] & 0x02) != 0;
}


uint16 rd16(const std::vector<uint8> &b, size_t at) { return (uint16)(b[at] | (b[at + 1] << 8)); }
uint32 rd32(const std::vector<uint8> &b, size_t at)
{
	return (uint32)b[at] | ((uint32)b[at + 1] << 8) | ((uint32)b[at + 2] << 16) | ((uint32)b[at + 3] << 24);
}


void put16(std::vector<uint8> &b, uint16 v) { b.push_back((uint8)v); b.push_back((uint8)(v >> 8)); }
void put32(std::vector<uint8> &b, uint32 v)
{
	for(  int i = 0;  i < 4;  i++  ) {
		b.push_back((uint8)(v >> (8 * i)));
	}
}


/// parse one node and its subtree; false if the file is malformed
bool parse_node(const std::vector<uint8> &b, size_t &at, pnode_t &n, int depth)
{
	if(  depth > 64  ||  at + 8 > b.size()  ) {
		return false;
	}
	n.start = at;
	memcpy(n.type, &b[at], 4);
	const uint16 nch = rd16(b, at + 4);
	uint32 size = rd16(b, at + 6);
	at += 8;
	if(  size == 0xFFFF  ) {
		if(  at + 4 > b.size()  ) {
			return false;
		}
		size = rd32(b, at);
		at += 4;
	}
	if(  size > b.size() - at  ) {
		return false;
	}
	n.body = at;
	n.body_len = size;
	at += size;
	n.kids.resize(nch);
	for(  uint16 i = 0;  i < nch;  i++  ) {
		if(  !parse_node(b, at, n.kids[i], depth + 1)  ) {
			return false;
		}
	}
	n.end = at;
	return true;
}


struct pak_t {
	std::vector<uint8> bytes;
	pnode_t root;
};


bool load_pak(const char *name, pak_t &pak)
{
	FILE *f = fopen(name, "rb");
	if(  f == NULL  ) {
		fprintf(stderr, "mres: cannot open %s\n", name);
		return false;
	}
	fseek(f, 0, SEEK_END);
	const long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	pak.bytes.resize(len > 0 ? (size_t)len : 0);
	const bool ok = len > 0  &&  fread(&pak.bytes[0], (size_t)len, 1, f) == 1;
	fclose(f);
	if(  !ok  ) {
		fprintf(stderr, "mres: cannot read %s\n", name);
		return false;
	}
	const uint8 *eof_mark = (const uint8 *)memchr(&pak.bytes[0], 0x1A, pak.bytes.size());
	if(  eof_mark == NULL  ) {
		fprintf(stderr, "mres: %s is not a pak (no header terminator)\n", name);
		return false;
	}
	size_t at = (size_t)(eof_mark - &pak.bytes[0]) + 1 + 4;   // header text, compiler version
	if(  !parse_node(pak.bytes, at, pak.root, 0)  ||  !is_type(pak.root, "ROOT")  ||  at != pak.bytes.size()  ) {
		fprintf(stderr, "mres: %s is not a single well-formed ROOT tree\n", name);
		return false;
	}
	return true;
}


/// one image, addressed the way a pairing addresses it
struct entry_t {
	std::string name;
	uint32 object_type;
	std::vector<uint16> path;
	const pnode_t *img;
	const pnode_t *companion;      ///< the XR32 sibling with img_index == this child, or NULL
};


bool entry_less(const entry_t &a, const entry_t &b)
{
	if(  a.name != b.name  ) {
		return a.name < b.name;
	}
	return a.path < b.path;
}


void collect(const pak_t &pak, const pnode_t &node, const entry_t &proto, std::vector<entry_t> &out)
{
	if(  proto.path.size() >= MRES_MAX_DEPTH  ) {
		return;
	}
	for(  size_t k = 0;  k < node.kids.size();  k++  ) {
		const pnode_t &c = node.kids[k];
		if(  is_type(c, "TEXT")  ||  is_type(c, "XREF")  ||  is_type(c, "XR32")  ) {
			continue;
		}
		entry_t e = proto;
		e.path.push_back((uint16)k);
		if(  memcmp(c.type, "IMG", 4) == 0  ) {
			e.img = &c;
			e.companion = NULL;
			for(  size_t j = 0;  j < node.kids.size();  j++  ) {
				const pnode_t &x = node.kids[j];
				if(  is_type(x, "XR32")  &&  x.body_len >= 20  &&  rd16(pak.bytes, x.body + 18) == (uint16)k  ) {
					e.companion = &x;
				}
			}
			out.push_back(e);
		}
		else {
			collect(pak, c, e, out);
		}
	}
}


/// every image under a named top-level object, the loader's own notion of identity
void catalogue(const pak_t &pak, std::vector<entry_t> &out)
{
	for(  size_t i = 0;  i < pak.root.kids.size();  i++  ) {
		const pnode_t &obj = pak.root.kids[i];
		if(  obj.kids.empty()  ||  !is_type(obj.kids[0], "TEXT")  ) {
			continue;
		}
		const pnode_t &t = obj.kids[0];
		const char *s = (const char *)&pak.bytes[t.body];
		const size_t n = strnlen(s, t.body_len);
		entry_t proto;
		proto.name.assign(s, n);
		proto.object_type = rd32(pak.bytes, obj.start);
		proto.img = NULL;
		proto.companion = NULL;
		collect(pak, obj, proto, out);
	}
}


std::vector<uint8> slice(const pak_t &pak, const pnode_t &n)
{
	return std::vector<uint8>(pak.bytes.begin() + n.start, pak.bytes.begin() + n.end);
}


void append_header(std::vector<uint8> &out, const char *type, uint16 nchildren, size_t body_len)
{
	out.insert(out.end(), type, type + 4);
	put16(out, nchildren);
	if(  body_len < 0xFFFF  ) {
		put16(out, (uint16)body_len);
	}
	else {
		put16(out, 0xFFFF);
		put32(out, (uint32)body_len);
	}
}

}   // namespace


int stlab_mres_main(const char *out_name, const char *base_name, const char *hires_name)
{
	pak_t base, hires;
	if(  !load_pak(base_name, base)  ||  !load_pak(hires_name, hires)  ) {
		return 1;
	}

	std::vector<entry_t> base_img, hires_img;
	catalogue(base, base_img);
	catalogue(hires, hires_img);
	std::stable_sort(base_img.begin(), base_img.end(), entry_less);

	// last one wins, which is how the loader resolves a duplicate name as well
	std::map<std::pair<std::string, std::vector<uint16> >, const entry_t *> hi;
	for(  size_t i = 0;  i < hires_img.size();  i++  ) {
		hi[std::make_pair(hires_img[i].name, hires_img[i].path)] = &hires_img[i];
	}

	std::vector<uint8> records;
	std::vector<std::vector<uint8> > img_children, xr_children;
	std::map<std::vector<uint8>, uint16> child_of;                   // dedup key -> child
	std::map<std::vector<uint8>, std::vector<uint8> > companion_of_img;   // ambiguity check
	uint16 count = 0;
	int skipped_version = 0, ambiguous = 0;

	for(  size_t i = 0;  i < base_img.size();  i++  ) {
		const entry_t &b = base_img[i];
		const std::map<std::pair<std::string, std::vector<uint16> >, const entry_t *>::const_iterator it =
			hi.find(std::make_pair(b.name, b.path));
		if(  it == hi.end()  ) {
			continue;
		}
		const entry_t &h = *it->second;

		// the guard: the base image exactly as the loader will see it (IMG v3)
		const pnode_t &bi = *b.img;
		if(  bi.body_len < 10  ||  base.bytes[bi.body + 6] != 3  ) {
			skipped_version++;
			continue;
		}
		const sint16 gx = (sint16)rd16(base.bytes, bi.body + 0);
		const sint16 gy = (sint16)rd16(base.bytes, bi.body + 2);
		const sint16 gw = (sint16)rd16(base.bytes, bi.body + 4);
		const sint16 gh = (sint16)rd16(base.bytes, bi.body + 7);
		const uint32 words = (uint32)((bi.body_len - 10) / 2);
		uLong adler = adler32(0L, Z_NULL, 0);
		if(  words > 0  ) {
			adler = adler32(adler, &base.bytes[bi.body + 10], words * 2);
		}

		// the child: the hires IMG, and its XR32 companion if there is one
		std::vector<uint8> img_raw = slice(hires, *h.img);
		std::vector<uint8> xr_raw;
		if(  h.companion  ) {
			xr_raw = slice(hires, *h.companion);
		}
		std::vector<uint8> key = img_raw;
		key.push_back(0xA5);
		if(  !xr_raw.empty()  ) {
			std::vector<uint8> k2 = xr_raw;
			const size_t idx_at = (h.companion->body - h.companion->start) + 18;
			k2[idx_at] = k2[idx_at + 1] = 0;             // the index is rewritten below
			if(  xr32_has_crc(*h.companion, k2)  ) {
				std::fill(k2.end() - 4, k2.end(), 0);    // and so is the CRC trailer
			}
			key.insert(key.end(), k2.begin(), k2.end());
		}
		uint16 child;
		const std::map<std::vector<uint8>, uint16>::const_iterator ci = child_of.find(key);
		if(  ci != child_of.end()  ) {
			child = ci->second;
		}
		else {
			if(  img_children.size() >= 0x7FFF  ) {
				fprintf(stderr, "mres: too many high-resolution images for one node\n");
				return 1;
			}
			child = (uint16)img_children.size();
			child_of[key] = child;
			img_children.push_back(img_raw);
			// two different companions for byte-identical IMG bodies: the loader
			// collapses the images to one id and only one companion can win
			std::map<std::vector<uint8>, std::vector<uint8> >::const_iterator ai = companion_of_img.find(img_raw);
			if(  ai != companion_of_img.end()  &&  ai->second != key  ) {
				ambiguous++;
				fprintf(stderr, "mres: warning: '%s' has a 2x image identical to another one but a different XR32; only one can be used\n", b.name.c_str());
			}
			companion_of_img[img_raw] = key;
			if(  !xr_raw.empty()  ) {
				const size_t idx_at = (h.companion->body - h.companion->start) + 18;
				xr_raw[idx_at] = (uint8)child;
				xr_raw[idx_at + 1] = (uint8)(child >> 8);
				if(  xr32_has_crc(*h.companion, xr_raw)  ) {
					// the trailer covers img_index, so it must follow the rewrite
					const size_t body_at = h.companion->body - h.companion->start;
					const size_t crc_at = xr_raw.size() - 4;
					const uint32 crc = (uint32)crc32( crc32(0L, Z_NULL, 0), &xr_raw[body_at], (uInt)(crc_at - body_at) );
					for(  int k = 0;  k < 4;  k++  ) {
						xr_raw[crc_at + k] = (uint8)(crc >> (8 * k));
					}
				}
				xr_children.push_back(xr_raw);
			}
		}

		records.push_back(1);                          // scale_log2: 2x
		records.push_back((uint8)b.path.size());
		put16(records, child);
		put32(records, b.object_type);
		put32(records, (uint32)adler);
		put16(records, (uint16)gx);
		put16(records, (uint16)gy);
		put16(records, (uint16)gw);
		put16(records, (uint16)gh);
		put32(records, words);
		put16(records, (uint16)b.name.size());
		records.insert(records.end(), b.name.begin(), b.name.end());
		for(  size_t d = 0;  d < b.path.size();  d++  ) {
			put16(records, b.path[d]);
		}
		if(  count == 0xFFFF  ) {
			fprintf(stderr, "mres: too many pairings for one node\n");
			return 1;
		}
		count++;
	}

	if(  count == 0  ) {
		fprintf(stderr, "mres: no image of %s has a counterpart in %s; nothing written\n", base_name, hires_name);
		return 1;
	}
	if(  base.root.kids.size() >= 0xFFFF  ) {
		fprintf(stderr, "mres: ROOT of %s has no room for another child\n", base_name);
		return 1;
	}

	std::vector<uint8> body;
	put16(body, MRES_WRITE_VERSION);
	put16(body, count);
	body.insert(body.end(), records.begin(), records.end());

	std::vector<uint8> out = base.bytes;
	const uint16 root_children = (uint16)(base.root.kids.size() + 1);
	out[base.root.start + 4] = (uint8)root_children;
	out[base.root.start + 5] = (uint8)(root_children >> 8);
	append_header(out, "MRES", (uint16)(img_children.size() + xr_children.size()), body.size());
	out.insert(out.end(), body.begin(), body.end());
	for(  size_t i = 0;  i < img_children.size();  i++  ) {
		out.insert(out.end(), img_children[i].begin(), img_children[i].end());
	}
	for(  size_t i = 0;  i < xr_children.size();  i++  ) {
		out.insert(out.end(), xr_children[i].begin(), xr_children[i].end());
	}

	FILE *f = fopen(out_name, "wb");
	if(  f == NULL  ||  fwrite(&out[0], out.size(), 1, f) != 1  ) {
		fprintf(stderr, "mres: cannot write %s\n", out_name);
		if(  f  ) fclose(f);
		return 1;
	}
	fclose(f);
	printf("mres: %u pairing(s), %u 2x image(s), %u XR32 companion(s), node %u bytes; %d skipped (IMG version), %d ambiguous\n",
		(unsigned)count, (unsigned)img_children.size(), (unsigned)xr_children.size(),
		(unsigned)(out.size() - base.bytes.size()), skipped_version, ambiguous);
	return 0;
}
