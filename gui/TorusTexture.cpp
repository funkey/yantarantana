#include "TorusTexture.h"

logger::LogChannel torustexturelog("torustexturelog", "[TorusTexture] ");

TorusTexture::TorusTexture(const util::box<int,2>& region) :
	_width (region.width() /TileSize + 10),
	_height(region.height()/TileSize + 10),
	_outOfDates(boost::extents[_width][_height]),
	_mapping(_width, _height),
	_texture(0) {

	LOG_DEBUG(torustexturelog) << "creating new torus texture with " << _width << "x" << _height << " tiles to cover " << region << std::endl;

	// the tiles cache has to contain at least the tiles that we need
	assert(_width  <= TilesCache::Width);
	assert(_height <= TilesCache::Height);

	reset(region.center());

	sg_gui::OpenGl::Guard guard;

	_texture = new sg_gui::Texture(_width*TileSize, _height*TileSize, GL_RGBA);

	for (unsigned int i = 0; i < TileSize*TileSize; i++)
		_notDoneImage[i] = sg_gui::skia_pixel_t(255, 0, 0, 255);

	_cache.setTileChangedCallback(boost::bind(&TorusTexture::onTileChacheChanged, this, _1));
}

TorusTexture::~TorusTexture() {

	sg_gui::OpenGl::Guard guard;

	if (_texture)
		delete _texture;
}

void
TorusTexture::reset(const util::point<int,2>& center) {

	LOG_DEBUG(torustexturelog) << "reseting torus texture around " << center << std::endl;

	// get the tile containing the center
	util::point<int,2> centerTile(getTileCoordinate(center.x()), getTileCoordinate(center.y()));

	// reset the tile mapping, such that all tiles around center map to 
	// [0,w)x[0,h)
	_mapping.reset(centerTile - util::point<int,2>(_width/2, _height/2));

	LOG_DEBUG(torustexturelog) << "new center tile is " << centerTile << std::endl;

	// reset the accumulated shift
	_shift = util::point<int,2>(0, 0);

	LOG_DEBUG(torustexturelog) << "current shift is " << _shift << std::endl;

	// center the cache around our center tile as well
	_cache.reset(util::point<int,2>(centerTile.x(), centerTile.y()));

	// mark all tiles as need-update
	for (unsigned int x = 0; x < _width; x++)
		for (unsigned int y = 0; y < _height; y++)
			_outOfDates[x][y] = true;
}

void
TorusTexture::shift(const util::point<int,2>& shift) {

	_shift += shift;

	LOG_ALL(torustexturelog) << "shifting texture content by " << shift << ", accumulated shift is " << _shift << std::endl;

	// We are shifting content out of the region covered by this texture.
	//
	// If we shifted far enough to the right, such that a whole column of tiles 
	// got shifted out of the region, we take this column and insert it at the 
	// left. We mark the newly inserted tiles as out-of-date, such that they get 
	// updated from the cache.

	while (_shift.x() >= (int)TileSize) {

		_mapping.shift(util::point<int,2>(-1, 0));
		_cache.shift(util::point<int,2>(1, 0));
		_shift.x() -= TileSize;

		// the new tiles are in the left column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int x = tilesRegion.min().x();
		for (int y = tilesRegion.min().y(); y < tilesRegion.max().y(); y++)
			markDirty(util::point<int,2>(x, y), OutOfDate);
	}
	while (_shift.x() <= -(int)TileSize) {

		_mapping.shift(util::point<int,2>(1, 0));
		_cache.shift(util::point<int,2>(-1, 0));
		_shift.x() += TileSize;

		// the new tiles are in the right column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int x = tilesRegion.max().x() - 1;
		for (int y = tilesRegion.min().y(); y < tilesRegion.max().y(); y++)
			markDirty(util::point<int,2>(x, y), OutOfDate);
	}
	while (_shift.y() >= (int)TileSize) {

		_mapping.shift(util::point<int,2>(0, -1));
		_cache.shift(util::point<int,2>(0, 1));
		_shift.y() -= TileSize;

		// the new tiles are in the top column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int y = tilesRegion.min().y();
		for (int x = tilesRegion.min().x(); x < tilesRegion.max().x(); x++)
			markDirty(util::point<int,2>(x, y), OutOfDate);
	}
	while (_shift.y() <= -(int)TileSize) {

		_mapping.shift(util::point<int,2>(0, 1));
		_cache.shift(util::point<int,2>(0, -1));
		_shift.y() += TileSize;

		// the new tiles are in the bottom column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int y = tilesRegion.max().y() - 1;
		for (int x = tilesRegion.min().x(); x < tilesRegion.max().x(); x++)
			markDirty(util::point<int,2>(x, y), OutOfDate);
	}

	LOG_ALL(torustexturelog) << "  cache region is now " << _mapping.get_region() << std::endl;
}

void
TorusTexture::markDirty(const util::box<int,2>& region, DirtyFlag dirtyFlag) {

	// get the tiles in the region
	util::box<int,2> tiles = getTiles(region);

	// mark them dirty
	for (int x = tiles.min().x(); x < tiles.max().x(); x++)
		for (int y = tiles.min().y(); y < tiles.max().y(); y++)
			markDirty(util::point<int,2>(x, y), dirtyFlag);
}

void
TorusTexture::render(const util::box<int,2>& region, Rasterizer& rasterizer) {

	LOG_ALL(torustexturelog) << "called render for " << region << std::endl;

	// get the tiles in the region
	util::box<int,2> tiles = getTiles(region);

	LOG_ALL(torustexturelog) << "tiles would be " << tiles << std::endl;

	// intersect it with the tiles that are used in the texture to make sure we 
	// are not drawing tiles that don't exist
	tiles = _mapping.get_region().intersection(tiles);

	LOG_ALL(torustexturelog) << "intersected with my tiles " << _mapping.get_region() << ", this gives " << tiles << std::endl;

	if (tiles.area() <= 0) {

		LOG_ALL(torustexturelog) << "I don't have tiles for this region" << std::endl;
		return;
	}

	// are all out-of-date tiles updated?
	bool allDone = false;

	// try at most two times to get all the tiles
	for (int i = 0; i < 2 && !allDone; i++) {

		// assume everything is okay
		allDone = true;

		// TODO: make this more efficient (by using a queue of reload requests)
		for (int x = tiles.min().x(); x < tiles.max().x(); x++)
			for (int y = tiles.min().y(); y < tiles.max().y(); y++) {

				util::point<int,2> tile(x, y);
				util::point<int,2> physicalTile = _mapping.map(tile);

				bool needUpdate = false;

				if (_cache.wasChanged(tile)) {

					LOG_ALL(torustexturelog) << "tile " << tile << " was changed in the cache" << std::endl;

					needUpdate = true;
					_cache.seenChange(tile);
				}

				if (_outOfDates[physicalTile.x()][physicalTile.y()]) {

					LOG_ALL(torustexturelog) << "tile " << tile << " is marked out-of-date" << std::endl;

					needUpdate = true;
				}

				if (needUpdate) {

					bool done = reloadTile(tile, physicalTile, rasterizer);

					allDone = allDone && done;
				}
			}
	}

	// draw the texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// The texture is in general split into four parts that we have to draw 
	// individually.

	util::box<int,2> subtiles[4];
	_mapping.split(tiles, subtiles);

	_texture->bind();

	// for each part...
	for (int i = 0; i < 4; i++) {

		LOG_ALL(torustexturelog) << "attempting to draw part " << i << " with tiles " << subtiles[i] << std::endl;

		if (subtiles[i].area() <= 0)
			continue;

		LOG_ALL(torustexturelog) << "drawing texture part " << i << std::endl;

		// the region covered by this part in pixels
		util::box<int,2> subregion = subtiles[i]*static_cast<int>(TileSize);

		util::point<int,2> physicalUpperLeft  = _mapping.map(subtiles[i].min());
		util::point<int,2> physicalLowerRight = physicalUpperLeft + util::point<int,2>(subtiles[i].width(), subtiles[i].height());

		// the texCoords as tiles in the texture
		util::box<double,2> texCoords(physicalUpperLeft.x(), physicalUpperLeft.y(), physicalLowerRight.x(), physicalLowerRight.y());
		// normalized to [0,1)x[0,1)
		texCoords /= util::point<double,2>(_width, _height);

#ifndef NDEBUG
		int r = rand()%255;
		int g = rand()%255;
		int b = 255;
		glColor3i(r, g, b);
		LOG_ALL(torustexturelog) << "rgb = " << r << " " << g << " " << b << std::endl;
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
#endif

		// draw the texture part
		glBegin(GL_QUADS);
		glTexCoord2d(texCoords.min().x(), texCoords.min().y()); glVertex2d(subregion.min().x(), subregion.min().y());
		glTexCoord2d(texCoords.max().x(), texCoords.min().y()); glVertex2d(subregion.max().x(), subregion.min().y());
		glTexCoord2d(texCoords.max().x(), texCoords.max().y()); glVertex2d(subregion.max().x(), subregion.max().y());
		glTexCoord2d(texCoords.min().x(), texCoords.max().y()); glVertex2d(subregion.min().x(), subregion.max().y());
		glEnd();
	}

	_texture->unbind();

	glDisable(GL_BLEND);
}

void
TorusTexture::setBackgroundRasterizer(std::shared_ptr<Rasterizer> rasterizer) {

	_cache.setBackgroundRasterizer(rasterizer);
}

util::box<int,2>
TorusTexture::getTiles(const util::box<int,2>& region) {

	util::box<int,2> tiles;

	tiles.min().x() = getTileCoordinate(region.min().x());
	tiles.min().y() = getTileCoordinate(region.min().y());
	tiles.max().x() = getTileCoordinate(region.max().x() - 1) + 1;
	tiles.max().y() = getTileCoordinate(region.max().y() - 1) + 1;

	return tiles;
}

int
TorusTexture::getTileCoordinate(int pixel) {

	return pixel/static_cast<int>(TileSize) - (pixel < 0 ? 1 : 0);
}

void
TorusTexture::markDirty(const util::point<int,2>& tile, DirtyFlag dirtyFlag) {

	LOG_ALL(torustexturelog) << "marking dirty tile " << tile << std::endl;

	// mark the tile out-of-date in the texture
	if (_mapping.get_region().contains(tile)) {

		LOG_ALL(torustexturelog) << "    marking out-of-date tile " << tile << std::endl;

		util::point<int,2> physicalTile = _mapping.map(tile);
		_outOfDates[physicalTile.x()][physicalTile.y()] = true;

		LOG_ALL(torustexturelog) << "    physical tile is " << physicalTile << std::endl;
	}

	// NeedsRedraw and NeedsUpdate have to be propagated to the cache
	if (dirtyFlag == NeedsRedraw)
		_cache.markDirty(tile, TilesCache::NeedsRedraw);
	else if (dirtyFlag == NeedsUpdate)
		_cache.markDirty(tile, TilesCache::NeedsUpdate);
}

bool
TorusTexture::reloadTile(const util::point<int,2>& tile, const util::point<int,2>& physicalTile, Rasterizer& rasterizer) {

	LOG_ALL(torustexturelog) << "reloading tile " << tile << std::endl;
	LOG_ALL(torustexturelog) << "    physical tile is " << physicalTile << std::endl;

	// get the tile's data (and update it on-the-fly, if needed)
	sg_gui::skia_pixel_t* data = _cache.getTile(tile, rasterizer);

	// get the target area within the texture
	util::box<int,2> textureRegion(physicalTile.x(), physicalTile.y(), physicalTile.x() + 1, physicalTile.y() + 1);
	textureRegion *= static_cast<int>(TileSize);

	// partially reload the texture
	if (data == 0) {

		LOG_ALL(torustexturelog) << "    tile is not ready, yet -- showing not-done image" << std::endl;

		_texture->loadData(_notDoneImage, textureRegion);

		return false;

	} else {

		_texture->loadData(data, textureRegion);

		// mark tile as up-to-date
		_outOfDates[physicalTile.x()][physicalTile.y()] = false;

		return true;
	}
}

void
TorusTexture::onTileChacheChanged(const util::point<int,2>& tile) {

	// TODO: re-enable without Slot
	//if (_contentChanged) {

		//if (_mapping.get_region().contains(tile)) {

			//LOG_ALL(torustexturelog) << "signalling content change" << std::endl;
			//(*_contentChanged)();
		//}
	//}
}
