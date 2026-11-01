/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "pakset_manager.h"

#include <map>
#include <vector>
#include <zlib.h>
#include "../descriptor/image.h"
#include "../descriptor/text_desc.h"
#include "../descriptor/reader/multires_reader.h"

#include "../utils/searchfolder.h"
#include "../display/simgraph.h"
#include "../sys/simsys.h"
#include "../simskin.h"
#include "../simloadingscreen.h"
#include "../descriptor/ground_desc.h"
#include "../descriptor/reader/obj_reader.h"
#include "../descriptor/vehicle_desc.h"
#include "../utils/simstring.h"
#include "../utils/cbuffer.h"
#include "../dataobj/translator.h"
#include "../gui/help_frame.h"
#include "../gui/simwin.h"
#include "../dataobj/environment.h"
#include "../network/pakset_info.h"

/* XR32 candidate nodes: the loader seam that parses them and pairs them with IMG.
 *
 * Only a 32 bit graphical build supplies the consumer this seam calls, and only
 * that build's image_t carries the fields it reads. In every other
 * configuration the loader behaves as it always did and skips the node. */
#if defined(COLOUR_DEPTH) && COLOUR_DEPTH == 32
#	include "../display/stlab_xr32_load.cci"
#else
static inline bool stlab_xr32_enabled() { return false; }
static inline void stlab_xr32_pair_candidates(obj_desc_t **, int, const obj_type *) { }
static inline bool stlab_xr32_read_candidate(FILE *, uint32) { return false; }
static inline bool stlab_xnex_read_candidate(FILE *, uint32) { return false; }
#endif


pakset_manager_t::obj_map_t*                                  pakset_manager_t::registered_readers;
inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t*> > pakset_manager_t::loaded;
pakset_manager_t::unresolved_map_t                            pakset_manager_t::unresolved;
ptrhashtable_tpl<obj_desc_t**, int>                           pakset_manager_t::fatals;
std::string                                                   pakset_manager_t::doublettes;
stringhashtable_tpl<missing_level_t>                          pakset_manager_t::missing_pak_names;
std::string                                                   pakset_manager_t::overlaid_warning;


static const char* missing_level_to_string[MISSING_WAY + 1] = {
	"",
	"factory",
	"vehicle",
	"sign",
	"wayobj",
	"bridge",
	"building",
	"way"
};

void pakset_manager_t::register_reader(obj_reader_t *reader)
{
	if(!registered_readers) {
		registered_readers = new obj_map_t;
	}

	registered_readers->put(reader->get_type(), reader);
	//printf("This program can read %s objects\n", get_type_name());
}


const char* pakset_manager_t::get_doubled_warning_message()
{
	return translator::translate("Doubled objects detected. Click to see details.");
}


void pakset_manager_t::load_pakset(bool load_addons)
{
	dbg->message("pakset_manager_t::load_pakset", "Reading object data from %s...", env_t::pak_dir.c_str());

	if (!load_paks_from_directory( env_t::pak_dir.c_str(), load_addons, translator::translate("Loading paks ...") )) {
		dbg->fatal("pakset_manager_t::load_pakset", "Failed to load pakset. Please re-download or select another pakset.");
	}

	overlaid_warning.clear();

	if(  had_overlaid()  ) {
		overlaid_warning.append( translator::translate("<h1>Error</h1><p><strong>") );
		overlaid_warning.append( env_t::pak_name + translator::translate("contains the following doubled objects:</strong><p>") + get_overlaid() + "<p>" );
		clear_overlaid();
	}

	if(  load_addons  ) {
		// try to read addons from private directory
		dr_chdir( env_t::user_dir );

		// Do not disable addons after failing to find addon objects
		// because a user might only have addon scenarios but no addon objects.
		// So do not check the return value here.
		load_paks_from_directory("addons/" + env_t::pak_name, true, translator::translate("Loading addon paks ..."));

		dr_chdir( env_t::base_dir );

		if(  had_overlaid()  ) {
			overlaid_warning.append( translator::translate("<h1>Warning</h1><p><strong>addons for") + env_t::pak_name + translator::translate("contains the following doubled objects:</strong><p>") + get_overlaid() );
			clear_overlaid();
		}
	}

	if (!finish_loading()) {
		dbg->fatal("pakset_manager_t::load_pakset", "Failed to load pakset. Please re-download or select another pakset.");
	}

	pakset_info_t::calculate_checksum();

	if(  env_t::pakset_debug  ) {
		pakset_info_t::debug();
	}
}

void pakset_manager_t::open_doubled_warning_window()
{
	help_frame_t *win = new help_frame_t();
	win->set_text( overlaid_warning.c_str() );
	create_win(win, w_info, magic_none);
}


bool pakset_manager_t::load_paks_from_directory(const std::string &path, bool load_addons, const char *message)
{
	const bool drawing = gfx->is_display_init();

	// step is a bitmask to decide when it's time to update the progress bar.
	// It takes the biggest power of 2 less than the number of elements and
	// divides it in 256 sub-steps at most (the -7 comes from here)

	searchfolder_t find;
	const searchfolder_t::search_flags_t addon_flags = load_addons ? searchfolder_t::SF_NONE : searchfolder_t::SF_NOADDONS;
	const sint32 max = find.search(path, "pak", addon_flags | searchfolder_t::SF_PREPEND_PATH, 4);
	sint32 step = -7;

	for(  sint32 bit = 1;  bit < max;  bit += bit  ) {
		step++;
	}
	if(  step < 0  ) {
		step = 0;
	}
	step = (2<<step)-1;

	if(drawing  &&  skinverwaltung_t::biglogosymbol==NULL) {
		const scr_size screen = gfx->get_screen_size();
		gfx->draw_rect( 0, 0, screen.w, screen.h, gfx->palette_lookup(COL_BLACK), true);

		if (!load_pak_file(path + "symbol.BigLogo.pak")) {
			dbg->warning("pakset_manager_t::load_paks_from_directory", "File 'symbol.BigLogo.pak' cannot be read, startup logo will not be displayed!");
		}
	}

	loadingscreen_t ls( message, max, true );

	if(  ground_desc_t::outside==NULL  ) {
		// define the pak tile width
		if (!load_pak_file(path + "ground.Outside.pak")) {
			return false;
		}

		if(ground_desc_t::outside==NULL) {
			dbg->warning("pakset_manager_t::load_paks_from_directory", "File ground.Outside.pak not found, cannot guess tile size! (driving on left will not work!)");
		}
		else if (char const* const copyright = ground_desc_t::outside->get_copyright()) {
			ls.set_info(copyright);
		}
	}

DBG_MESSAGE("pakset_manager_t::load_paks_from_directory", "Reading from '%s'", path.c_str());

	uint n = 0;
	for (char* const& pak_filename : find) {
		if (!load_pak_file(pak_filename)) {
			dbg->warning("pakset_manager_t::load_paks_from_directory", "Cannot load '%s', some objects might be unavailable!", pak_filename);
		}

		if ((n++ & step) == 0 && drawing) {
			ls.set_progress(n);
		}
	}

	ls.set_progress(max);
	return find.begin()!=find.end();
}


bool pakset_manager_t::load_pak_file(const std::string &filename)
{
	// added trace
	PAKSET_INFO("loading", "name=%s", filename.c_str());

	FILE* const fp = dr_fopen(filename.c_str(), "rb");
	if (!fp) {
		dbg->error("pakset_manager_t::load_pak_file", "Reading '%s' failed!", filename.c_str());
		return false;
	}

	// This is the normal header reading code
	int c;
	uint32 n = 0;

	do {
		c = fgetc(fp);
		n ++;
	} while(c != EOF && c != 0x1a);

	if(c == EOF) {
		dbg->error("pakset_manager_t::load_pak_file", "Unexpected end of file after %u bytes while reading '%s'!", n, filename.c_str());
		fclose(fp);
		return false;
	}

	// Compiled Version
	char dummy[4];
	n = fread(dummy, 4, 1, fp);
	if (n != 1) {
		fclose(fp);
		return false;
	}

	char *p = dummy;
	const uint32 version = decode_uint32(p);

	PAKSET_INFO("pakset_manager_t::load_pak_file", "%s, reading %u blocks, file version is %x", filename.c_str(), n, version);

	if(version <= COMPILER_VERSION_CODE) {
		obj_desc_t *data = NULL;
		if (!read_nodes(fp, data, 0, version)) {
			fclose(fp);
			return false;
		}
	}
	else {
		dbg->warning("pakset_manager_t::load_pak_file", "Version of '%s' is too old, %u instead of %u", filename.c_str(), version, COMPILER_VERSION_CODE );
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}


/*
 * Do the last loading procedures
 * Resolve all xrefs
 */
bool pakset_manager_t::finish_loading()
{
	// vehicle to follow to mark something cannot lead a convoi (prev[0]=any) or cannot end a convoi (next[0]=any)
	vehicle_desc_t::any_vehicle = new vehicle_desc_t(ignore_wt, 1, vehicle_desc_t::unknown);

	// first we add the any_vehicle to xrefs
	obj_for_xref( obj_vehicle, "any", vehicle_desc_t::any_vehicle );

	resolve_xrefs();
	// after the xrefs, so object replacement has settled
	resolve_multires();

	for(auto const& elem : *registered_readers) {
		PAKSET_INFO("pakset_manager_t::finish_loading", "Checking %s objects...", elem.value->get_type_name());

		if (!elem.value->successfully_loaded()) {
			dbg->warning("pakset_manager_t::finish_loading", "... failed!");
			return false;
		}
	}

	return true;
}


void pakset_manager_t::clear_missing_paks()
{
	missing_pak_names.clear();
}

// store missing obj during load and their severity
void pakset_manager_t::add_missing_paks( const char *name, missing_level_t level )
{
	if(  missing_pak_names.get( name )==NOT_MISSING  ) {
		missing_pak_names.put( strdup(name), level );
		dbg->warning("pakset_manager_t::add_missing_paks()", "Missing %s name %s will be replaced.", missing_level_to_string[level], name);
	}
}


void pakset_manager_t::warn_if_paks_missing()
{
	if(  missing_pak_names.empty()  ) {
		return; // everything OK
	}

	cbuffer_t msg;
	msg.append("<title>");
	msg.append(translator::translate("Missing pakfiles"));
	msg.append("</title>\n");

	cbuffer_t error_paks;
	cbuffer_t warning_paks;

	cbuffer_t paklog;
	paklog.append( "\n" );
	for(auto const& i : missing_pak_names) {
		if (i.value <= MISSING_ERROR) {
			error_paks.append(translator::translate(i.key));
			error_paks.append("<br>\n");
			paklog.append( i.key );
			paklog.append("\n" );
		}
		else {
			warning_paks.append(translator::translate(i.key));
			warning_paks.append("<br>\n");
		}
	}

	if(  error_paks.len()>0  ) {
		msg.append("<h1>");
		msg.append(translator::translate("Pak which may cause severe errors:"));
		msg.append("</h1><br>\n");
		msg.append("<br>\n");
		msg.append( error_paks );
		msg.append("<br>\n");
		dbg->warning( "The following paks are missing and may cause errors", paklog );
	}

	if(  warning_paks.len()>0  ) {
		msg.append("<h1>");
		msg.append(translator::translate("Pak which may cause visual errors:"));
		msg.append("</h1><br>\n");
		msg.append("<br>\n");
		msg.append( warning_paks );
		msg.append("<br>\n");
	}

	help_frame_t *win = new help_frame_t();
	win->set_text( msg );
	create_win(win, w_info, magic_pakset_info_t);
}


static bool read_node_info(obj_node_info_t& node, FILE* const f, uint32 const version)
{
	char data[EXT_OBJ_NODE_INFO_SIZE];
	if (fread(data, OBJ_NODE_INFO_SIZE, 1, f) != 1) {
		return false;
	}

	char *p = data;
	node.type      = decode_uint32(p);
	node.nchildren = decode_uint16(p);
	node.size      = decode_uint16(p);

	// can have larger records
	if (version != COMPILER_VERSION_CODE_11 && node.size == LARGE_RECORD_SIZE) {
		if (fread(p, sizeof(data) - OBJ_NODE_INFO_SIZE, 1, f) != 1) {
			return false;
		}
		node.size = decode_uint32(p);
	}

	return true;
}


/*
 * What each parent's children were read as. Only kept between reading and
 * resolve_multires(): a high-resolution pairing walks a path of child indices, and every
 * step has to be checked against this rather than trusted (obj_desc_t does not store how
 * many children it has).
 */
static std::map<const obj_desc_t *, std::vector<obj_type> > *multires_kinds = NULL;


static void multires_note_children(const obj_desc_t *parent, const std::vector<obj_type> &kinds)
{
	if(  parent == NULL  ||  kinds.empty()  ) {
		return;
	}
	if(  multires_kinds == NULL  ) {
		multires_kinds = new std::map<const obj_desc_t *, std::vector<obj_type> >();
	}
	(*multires_kinds)[parent] = kinds;
}


bool pakset_manager_t::read_nodes(FILE *fp, obj_desc_t *&data, int node_depth, uint32 version, obj_type *type_read)
{
	obj_node_info_t node;
	if (!read_node_info(node, fp, version)) {
		return false;
	}
	// so a caller can tell whether an object's first child is its name
	if (type_read) { *type_read = static_cast<obj_type>(node.type); }

	obj_reader_t *reader = registered_readers->get(static_cast<obj_type>(node.type));

	if(reader) {
//		PAKSET_INFO("pakset_manager_t::read_nodes", "Reading %.4s-node of length %d with '%s'", reinterpret_cast<const char *>(&node.type), node.size, reader->get_type_name());
		data = reader->read_node(fp, node);

		if (!data) {
			return false;
		}

		obj_type first_child = obj_root;
		std::vector<obj_type> child_kinds(node.nchildren, obj_root);   // what each child was read as
		if (node.nchildren != 0) {
			data->children = new obj_desc_t *[node.nchildren];

			for (int i = 0; i < node.nchildren; i++) {
				obj_type child_type = obj_root;
				if (!read_nodes(fp, data->children[i], node_depth + 1, version, &child_type)) {
					// Note: cannot delete siblings of data->children[i], since equal images point to the same desc
					delete data; // data->children is delete[]'d by the destructor
					data = NULL;
					return false;
				}
				// A high-resolution image list may only hold images. A damaged file
				// can put anything there, and the pairing must not end up casting it
				// to one, so anything else is dropped here where the type is known.
				// (MRES v2 also allows XR32 companions after the images; those have
				// no reader, are consumed by the XR32 seam and leave a NULL slot, so
				// this rule does not disturb them.)
				if (i == 0) { first_child = child_type; }
				child_kinds[i] = child_type;
				if (node.type == obj_multires && child_type != obj_image) {
					data->children[i] = NULL;
				}
			}
		}

//DBG_DEBUG("obj_reader_t","registering with '%s'", reader->get_type_name());
		// A high-resolution pairing names its base object, so the names have
		// to be recallable later. An object node carries its name in its
		// first child, and this is where the object and that child are both
		// in hand.
		if(  node_depth == 1  &&  node.nchildren > 0  &&  first_child == obj_text  &&  data->children  ) {
			multires_note_object( static_cast<const text_desc_t *>(data->children[0])->get_text(), static_cast<obj_type>(node.type), data );
		}

		multires_note_children( data, child_kinds );

		// pair any candidate node parsed while reading these children
		if(  stlab_xr32_enabled()  ) {
			stlab_xr32_pair_candidates( data->children, node.nchildren, child_kinds.empty() ? NULL : &child_kinds[0] );
		}

		if(node_depth<2  ||  node.type!=obj_cursor) {
			// since many buildings are with cursors that do not need registration
			reader->register_obj(data);
		}
	}
	else {
		// no reader found ...
		// consume the candidate node instead of skipping it
		bool stlab_consumed = false;
		if(  stlab_xr32_enabled()  &&  memcmp(&node.type, "XR32", 4) == 0  ) {
			stlab_consumed = stlab_xr32_read_candidate( fp, node.size );
		}
		// XR32-PRISSI-NIGHT-IMAGE-PROTOTYPE-01: the experimental night node, same seam
		else if(  stlab_xr32_enabled()  &&  memcmp(&node.type, "XNEX", 4) == 0  ) {
			stlab_consumed = stlab_xnex_read_candidate( fp, node.size );
		}
		if(  !stlab_consumed  ) {
			dbg->warning("pakset_manager_t::read_nodes", "Skipping unknown %.4s-node\n", reinterpret_cast<const char *>(&node.type));
			if (fseek(fp, node.size, SEEK_CUR) != 0) {
				return false;
			}
		}

		for(int i = 0; i < node.nchildren; i++) {
			if (!skip_nodes(fp,version)) {
				return false;
			}
		}

		data = NULL;
	}

	return true;
}


bool pakset_manager_t::skip_nodes(FILE *fp,uint32 version)
{
	obj_node_info_t node;
	if (!read_node_info(node, fp, version)) {
		return false;
	}

	if (fseek(fp, node.size, SEEK_CUR) != 0) {
		return false;
	}

	for(int i = 0; i < node.nchildren; i++) {
		if (!skip_nodes(fp,version)) {
			return false;
		}
	}

	return true;
}


/*
 * High-resolution counterparts.
 *
 * This runs late on purpose. Pak files are enumerated unsorted, and a later pak
 * may replace an object of the same name, so nothing may be bound while paks
 * are still being read. By the time resolve_xrefs() has run, replacement has
 * settled and the answer no longer depends on the order the files arrived in.
 *
 * A pairing names its base by the owning object's name and the path of child
 * indices to the image, and carries what that image looked like when the
 * pairing was made. A name alone would not be safe: duplicate names resolve to
 * the last one loaded, so an add-on can keep the name and change the art, and
 * the guard is what stops a counterpart built for the old art being used for
 * the new.
 */

static std::vector<mres_pair_t> *multires_pending = NULL;
// by node type, then name, like `loaded`: pak64.german has a cursor AND a menu called GeneralTools, and with
// name alone the pairing found whichever of the two the directory listed last
static inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t *> > *multires_objects = NULL;


void pakset_manager_t::multires_add(const mres_pair_t &pair)
{
	if(  multires_pending == NULL  ) {
		multires_pending = new std::vector<mres_pair_t>();
	}
	multires_pending->push_back( pair );
}


void pakset_manager_t::multires_bind_images(obj_desc_t *mres)
{
	// The reader could only carry the child index, since the images are read
	// after it returns. They exist now.
	if(  multires_pending == NULL  ) {
		return;
	}
	for(  size_t i = 0;  i < multires_pending->size();  i++  ) {
		mres_pair_t &pair = (*multires_pending)[i];
		if(  pair.bound  ||  pair.hires == IMG_EMPTY  ) {
			continue;
		}
		obj_desc_t *child = mres->children ? mres->children[pair.hires] : NULL;
		pair.hires = child ? static_cast<image_t *>(child)->imageid : IMG_EMPTY;
		pair.bound = true;
	}
}


void pakset_manager_t::multires_note_object(const char *name, obj_type type, obj_desc_t *desc)
{
	if(  name == NULL  ||  *name == 0  ) {
		return;
	}
	if(  multires_objects == NULL  ) {
		multires_objects = new inthashtable_tpl<obj_type, stringhashtable_tpl<obj_desc_t *> >();
	}
	stringhashtable_tpl<obj_desc_t *> *of_type = multires_objects->access( type );
	if(  of_type == NULL  ) {
		multires_objects->put( type );
		of_type = multires_objects->access( type );
	}
	// last one of this type wins, which is how a duplicate object name resolves anyway
	of_type->set( name, desc );
}


void pakset_manager_t::resolve_multires()
{
	if(  multires_pending == NULL  ) {
		// nothing to pair: the table of child kinds is not needed either
		delete multires_kinds;
		multires_kinds = NULL;
		return;
	}
#if COLOUR_DEPTH != 0
	for(  size_t i = 0;  i < multires_pending->size();  i++  ) {
		const mres_pair_t &pair = (*multires_pending)[i];
		// `bound` is only set once the node's images have been read and the
		// carried child index turned into an image id. A node that failed part
		// way through never gets that far, and its pairings still hold indices -
		// which must not be mistaken for image ids.
		if(  !pair.bound  ||  pair.hires == IMG_EMPTY  ) {
			continue;
		}
		const stringhashtable_tpl<obj_desc_t *> *of_type = multires_objects ? multires_objects->access( pair.object_type ) : NULL;
		obj_desc_t *obj = of_type ? of_type->get( pair.base_object.c_str() ) : NULL;
		if(  obj == NULL  ) {
			dbg->warning( "pakset_manager_t::resolve_multires", "'%s' is not in this pakset; its high-resolution image is unused", pair.base_object.c_str() );
			continue;
		}

		// walk to the image, keeping the slot so it can be repointed below
		obj_desc_t **slot = NULL;
		obj_desc_t *cur = obj;
		bool on_image = false;
		for(  uint8 d = 0;  d < pair.path_depth;  d++  ) {
			const std::map<const obj_desc_t *, std::vector<obj_type> >::const_iterator k =
				(cur != NULL  &&  multires_kinds != NULL) ? multires_kinds->find( cur ) : std::map<const obj_desc_t *, std::vector<obj_type> >::const_iterator();
			if(  cur == NULL  ||  cur->children == NULL  ||  multires_kinds == NULL  ||  k == multires_kinds->end()
			     ||  pair.path[d] >= k->second.size()  ) {
				slot = NULL;
				break;
			}
			on_image = k->second[ pair.path[d] ] == obj_image;
			slot = &cur->children[ pair.path[d] ];
			cur = *slot;
		}
		if(  slot == NULL  ||  cur == NULL  ||  !on_image  ) {
			dbg->warning( "pakset_manager_t::resolve_multires", "the image path for '%s' leads nowhere; its high-resolution image is unused", pair.base_object.c_str() );
			continue;
		}

		image_t *base = static_cast<image_t *>(cur);

		// the guard: shape first, then the pixels
		bool same = base->x == pair.guard_x  &&  base->y == pair.guard_y  &&
		            base->w == pair.guard_w  &&  base->h == pair.guard_h  &&
		            (uint32)base->len == pair.guard_len;
		if(  same  &&  base->len > 0  &&  base->data != NULL  ) {
			// over the words as serialised little-endian - the bytes of the IMG
			// body on disk - so the check does not depend on host byte order
			uLong a = adler32( 0L, NULL, 0 );
			uint8 le[512];
			for(  uint32 w = 0;  w < (uint32)base->len;  ) {
				uint32 k = 0;
				for(  ;  k < sizeof(le) / 2  &&  w < (uint32)base->len;  k++, w++  ) {
					le[2 * k]     = (uint8)( base->data[w] & 0xFF );
					le[2 * k + 1] = (uint8)( base->data[w] >> 8 );
				}
				a = adler32( a, (const Bytef *)le, 2 * k );
			}
			same = (uint32)a == pair.guard_adler;
		}
		if(  !same  ) {
			dbg->warning( "pakset_manager_t::resolve_multires", "'%s' is not the image its high-resolution counterpart was made for; using the authored one", pair.base_object.c_str() );
			continue;
		}

		const image_id attached = simgraph_get_hires( base->imageid );
		if(  attached != IMG_EMPTY  ) {
			if(  attached == pair.hires  ) {
				continue;   // the same pairing twice; nothing to do
			}
			// Two logically different images share one id because their pixels
			// are identical, and they want different counterparts. Give this
			// one an image of its own so both can be right.
			image_t *split = image_t::copy_image( *base );
			split->register_image();
			*slot = split;
			base = split;
		}
		simgraph_set_hires( base->imageid, pair.hires );
	}
#endif
	delete multires_pending;
	multires_pending = NULL;
	delete multires_objects;
	multires_objects = NULL;
	delete multires_kinds;
	multires_kinds = NULL;
}


void pakset_manager_t::resolve_xrefs()
{
	slist_tpl<obj_desc_t *> xref_nodes;

	for(auto const& u : unresolved) {
		for(auto const& i : u.value) {
			obj_desc_t *obj_loaded = NULL;

			if (!strempty(i.key)) {
				if (stringhashtable_tpl<obj_desc_t*>* const objtype_loaded = loaded.access(u.key)) {
					obj_loaded = objtype_loaded->get(i.key);
				}
			}

			for(obj_desc_t** const x : i.value) {
				if (!obj_loaded && fatals.get(x)) {
					dbg->fatal("pakset_manager_t::resolve_xrefs", "Cannot resolve '%4.4s-%s'", &u.key, i.key);
				}
				// delete old xref-node
				xref_nodes.append(*x);
				*x = obj_loaded;
			}
		}
	}

	while (!xref_nodes.empty()) {
		delete xref_nodes.remove_first();
	}

	loaded.clear();
	unresolved.clear();
	fatals.clear();
}


void pakset_manager_t::obj_for_xref(obj_type type, const char *name, obj_desc_t *data)
{
	stringhashtable_tpl<obj_desc_t *> *objtype_loaded = loaded.access(type);

	if(!objtype_loaded) {
		loaded.put(type);
		objtype_loaded = loaded.access(type);
	}
	objtype_loaded->remove(name);
	objtype_loaded->put(name, data);
}


void pakset_manager_t::xref_to_resolve(obj_type type, const char *name, obj_desc_t **dest, bool fatal)
{
	stringhashtable_tpl< slist_tpl<obj_desc_t **> > *typeunresolved = unresolved.access(type);

	if(!typeunresolved) {
		unresolved.put(type);
		typeunresolved = unresolved.access(type);
	}
	slist_tpl<obj_desc_t **> *list = typeunresolved->access(name);
	if(!list) {
		typeunresolved->put(name);
		list = typeunresolved->access(name);
	}
	list->insert(dest);
	if(fatal) {
		fatals.put(dest, 1);
	}
}


void pakset_manager_t::doubled(const char *what, const char *name)
{
	doublettes.append( (std::string)what+"::"+name+"<br/>" );
}
