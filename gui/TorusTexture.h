#ifndef YANTA_GUI_TORUS_TEXTURE_H__
#define YANTA_GUI_TORUS_TEXTURE_H__

#include <sg_gui/Texture.h>

#include <util/torus_mapping.hpp>
#include "Rasterizer.h"
#include "TilesCache.h"

/**
 * A texture that can efficiently shift its content by wrapping it around the 
 * edges. This is done virtually by splitting the texture according to a torus 
 * mapping from logical (content) coordinates to physical (texture) coordinates.
 */
class TorusTexture {

public:

	/**
	 * Flags to pass to markDirty() to indicate different types of dirtiness.
	 */
	enum DirtyFlag {

		// texture needs to be reload from cache
		OutOfDate,

		// needs a (possibly incremental) update
		NeedsUpdate,

		// needs a complete redraw
		NeedsRedraw
	};

	/**
	 * Create a torus texture covering and representing at least the given 
	 * region.
	 */
	TorusTexture(const util::box<int,2>& region);

	~TorusTexture();

	/**
	 * Reset the texture to represent the area around pixel 'center'.
	 */
	void reset(const util::point<int,2>& center);

	/**
	 * Shift the content of the texture by the given amount.
	 */
	void shift(const util::point<int,2>& shift);

	/**
	 * Mark a region of the texture as dirty.
	 */
	void markDirty(const util::box<int,2>& region, DirtyFlag dirtyFlag);

	/**
	 * Render the content of the texture for region into region. If parts of the 
	 * texture are dirty, they will be updated using the provided painter.
	 */
	void render(const util::box<int,2>& region, Rasterizer& rasterizer);

	/**
	 * Set a painter for the background clean-up thread.
	 */
	void setBackgroundRasterizer(std::shared_ptr<Rasterizer> rasterizer);

private:

	/**
	 * Get all tiles intersecting the given region.
	 */
	util::box<int,2> getTiles(const util::box<int,2>& region);

	/**
	 * Get the tile coordinate of a pixel coordinate.
	 */
	int getTileCoordinate(int pixel);

	/**
	 * Mark a tile in logical coordinates as dirty.
	 */
	void markDirty(const util::point<int,2>& tile, DirtyFlag dirtyFlag);

	/**
	 * Try to reload a tile. Returns false, if the tile is not present in the 
	 * cache, yet.
	 */
	bool reloadTile(const util::point<int,2>& tile, const util::point<int,2>& physicalTile, Rasterizer& rasterizer);

	/**
	 * Callback for the tiles cache.
	 */
	void onTileChacheChanged(const util::point<int,2>& tile);

	// accumulated shift in pixels
	util::point<int,2> _shift;

	// Internally, the torus texture is created from a set of tiles. These tiles 
	// come from a cache. Almost all operations are performed with respect to 
	// these tiles (like remembering dirty regions).

	// the size of the tiles the buffer provides
	static const unsigned int TileSize = TilesCache::TileSize;

	// the width and height of the texture in tiles
	unsigned int _width;
	unsigned int _height;

	// 2D array of out-of-date flags for the tiles of the texture
	typedef boost::multi_array<bool, 2> out_of_dates_type;
	out_of_dates_type _outOfDates;

	// mapping from logical tile coordinats to phsical tile coordinates
	torus_mapping<int> _mapping;

	// the cache to get tiles from
	TilesCache _cache;

	// the actual OpenGl texture
	sg_gui::Texture* _texture;

	// the image to show for tiles that haven't been rendered, yet
	sg_gui::skia_pixel_t _notDoneImage[TileSize*TileSize];
};

#endif // YANTA_GUI_TORUS_TEXTURE_H__

