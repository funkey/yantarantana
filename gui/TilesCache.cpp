#include <boost/timer/timer.hpp>

#include <SkCanvas.h>
#include <SkBitmap.h>

#include <util/Logger.h>
#include "TilesCache.h"

logger::LogChannel tilescachelog("tilescachelog", "[TilesCache] ");

TilesCache::TilesCache(const util::point<int,2>& center) :
	_tiles(boost::extents[Width][Height][TileSize*TileSize]),
	_tileStates(boost::extents[Width][Height]),
	_tileChanged(boost::extents[Width][Height]),
	_backgroundRasterizerStopped(false),
	_backgroundThread(boost::bind(&TilesCache::cleanUp, this)) {

	LOG_ALL(tilescachelog) << "creating new tiles cache around tile " << center << std::endl;

	for (unsigned int x = 0; x < Width; x++)
		for (unsigned int y = 0; y < Height; y++)
			_tileChanged[x][y] = false;

	reset(center);
}

TilesCache::~TilesCache() {

	LOG_ALL(tilescachelog) << "tearing background thread down..." << std::endl;

	_backgroundRasterizerStopped = true;
	_tileChangedCallback = boost::function<void(const util::point<int,2>&)>();
	_wakeupBackgroundRasterizer.notify_one();
	_backgroundThread.join();

	LOG_ALL(tilescachelog) << "background thread stopped" << std::endl;
}

void
TilesCache::reset(const util::point<int,2>& center) {

	_mappingVersionTag.lock();

	// reset the tile mapping, such that all tiles around center map to 
	// [0,w)x[0,h)
	_mapping.reset(center - util::point<int,2>(Width/2, Height/2));

	_mappingVersionTag.unlock();

	// TODO:
	// • this doesn't need to follow a spiral anymore
	for (unsigned int x = 0; x < Width; x++)
		for (unsigned int y = 0; y < Height; y++)
			markDirtyPhysical(util::point<int,2>(x, y), Invalid);
}

void
TilesCache::shift(const util::point<int,2>& shift) {

	LOG_ALL(tilescachelog) << "shifting cache content by " << shift << " tiles" << std::endl;

	// We are shifting content out of the region covered by this cache.
	//
	// If we shifted far enough to the right, such that a whole column of tiles 
	// got shifted out of the region, we take this column and insert it at the 
	// left. We mark the newly inserted tiles as dirty.

	util::point<int,2> remaining = shift;

	while (remaining.x() > 0) {

		_mappingVersionTag.lock();

		_mapping.shift(util::point<int,2>(-1, 0));
		remaining.x()--;

		_mappingVersionTag.unlock();

		// the new tiles are in the left column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int x = tilesRegion.min().x();
		for (int y = tilesRegion.min().y(); y < tilesRegion.max().y(); y++)
			markDirty(util::point<int,2>(x, y), Invalid);
	}
	while (remaining.x() < 0) {

		_mappingVersionTag.lock();

		_mapping.shift(util::point<int,2>(1, 0));
		remaining.x()++;

		_mappingVersionTag.unlock();

		// the new tiles are in the right column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int x = tilesRegion.max().x() - 1;
		for (int y = tilesRegion.min().y(); y < tilesRegion.max().y(); y++)
			markDirty(util::point<int,2>(x, y), Invalid);
	}
	while (remaining.y() > 0) {

		_mappingVersionTag.lock();

		_mapping.shift(util::point<int,2>(0, -1));
		remaining.y()--;

		_mappingVersionTag.unlock();

		// the new tiles are in the top column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int y = tilesRegion.min().y();
		for (int x = tilesRegion.min().x(); x < tilesRegion.max().x(); x++)
			markDirty(util::point<int,2>(x, y), Invalid);
	}
	while (remaining.y() < 0) {

		_mappingVersionTag.lock();

		_mapping.shift(util::point<int,2>(0, 1));
		remaining.y()++;

		_mappingVersionTag.unlock();

		// the new tiles are in the bottom column
		util::box<int,2> tilesRegion = _mapping.get_region();
		int y = tilesRegion.max().y() - 1;
		for (int x = tilesRegion.min().x(); x < tilesRegion.max().x(); x++)
			markDirty(util::point<int,2>(x, y), Invalid);
	}

	LOG_ALL(tilescachelog) << "cache region is now " << _mapping.get_region() << std::endl;
}

void
TilesCache::markDirty(const util::point<int,2>& tile, TileState state) {

	LOG_ALL(tilescachelog) << "marking tile " << tile << " as " << (state == NeedsUpdate ? "needs update" : "needs redraw") << std::endl;;

	if (!_mapping.get_region().contains(tile)) {

		LOG_ALL(tilescachelog) << " -- this tile is not in the cache" << std::endl;
		return;
	}

	util::point<int,2> physicalTile = _mapping.map(tile);

	markDirtyPhysical(physicalTile, state);
}

void
TilesCache::markDirtyPhysical(const util::point<int,2>& physicalTile, TileState state) {

	// without a background clean-up thread, allow no invalid flags
	if (!_backgroundRasterizer && state == Invalid)
		state = NeedsRedraw;

	// set the flag, but make sure we are not overwriting previous dirty flags 
	// of higher precedence
	_tileStates[physicalTile.x()][physicalTile.y()] = std::max(_tileStates[physicalTile.x()][physicalTile.y()], state);

	if (_backgroundRasterizer) {

		{
			boost::lock_guard<boost::mutex> lock(_haveDirtyTilesMutex);
			_haveDirtyTiles = true;
		}

		_wakeupBackgroundRasterizer.notify_one();
	}
}

sg_gui::skia_pixel_t*
TilesCache::getTile(const util::point<int,2>& tile, Rasterizer& rasterizer) {

	LOG_ALL(tilescachelog) << "getting tile " << tile << std::endl;

	util::point<int,2> physicalTile = _mapping.map(tile);

	//boost::mutex::scoped_lock lock(_tileMutexes[physicalTile.x()][physicalTile.y()]);

	// the tile is not ready, yet
	if (_tileStates[physicalTile.x()][physicalTile.y()] == Invalid)
		return 0;

	if (_tileStates[physicalTile.x()][physicalTile.y()] == NeedsUpdate) {

		LOG_ALL(tilescachelog) << "this tile needs an update" << std::endl;

		// get the region covered by the tile in pixels
		util::box<int,2> tileRegion(tile.x(), tile.y(), tile.x() + 1, tile.y() + 1);
		tileRegion *= static_cast<int>(TileSize);

		updateTile(physicalTile, tileRegion, rasterizer);
	}

	if (_tileStates[physicalTile.x()][physicalTile.y()] == NeedsRedraw) {

		LOG_ALL(tilescachelog) << "this tile needs a redraw" << std::endl;

		// get the region covered by the tile in pixels
		util::box<int,2> tileRegion(tile.x(), tile.y(), tile.x() + 1, tile.y() + 1);
		tileRegion *= static_cast<int>(TileSize);

		rasterizer.setIncremental(false);
		updateTile(physicalTile, tileRegion, rasterizer);
		rasterizer.setIncremental(true);
	}

	return &_tiles[physicalTile.x()][physicalTile.y()][0];
}

bool
TilesCache::wasChanged(const util::point<int,2>& tile) {

	util::point<int,2> physicalTile = _mapping.map(tile);

	return _tileChanged[physicalTile.x()][physicalTile.y()];
}

void
TilesCache::seenChange(const util::point<int,2>& tile) {

	util::point<int,2> physicalTile = _mapping.map(tile);

	_tileChanged[physicalTile.x()][physicalTile.y()] = false;
}

void
TilesCache::setBackgroundRasterizer(std::shared_ptr<Rasterizer> rasterizer) {

	_backgroundRasterizer = rasterizer;
}

void
TilesCache::updateTile(const util::point<int,2>& physicalTile, const util::box<int,2>& tileRegion, Rasterizer& rasterizer) {

	LOG_ALL(tilescachelog) << "updating physical tile " << physicalTile << " with content of " << tileRegion << std::endl;

	// It can happen that a clean-up request became stale because getTile() 
	// cleaned the tile already. In this case, there is nothing to do here.
	if (_tileStates[physicalTile.x()][physicalTile.y()] == Clean) {

		LOG_ALL(tilescachelog) << "this tile is clean already -- skip update" << std::endl;
		return;
	}

	// Possible data race:
	// 
	// Tile get's cleaned by someone else while we are here.

	// mark it as clean
	_tileStates[physicalTile.x()][physicalTile.y()] = Clean;

	// get the data of the tile
	sg_gui::skia_pixel_t* buffer = &_tiles[physicalTile.x()][physicalTile.y()][0];

	// wrap the buffer in a skia bitmap
	SkBitmap bitmap;
	bitmap.setInfo(SkImageInfo::MakeN32Premul(TileSize, TileSize));
	bitmap.setPixels(buffer);

	SkCanvas canvas(bitmap);

	// Now, we have a surface of widthxheight, with (0,0) being the upper left 
	// corner and (width-1,height-1) the lower right. Translate operations, such 
	// that the upper left is tileRegion.upperLeft() and lower right is 
	// tileRegion.lowerRight().

	// translate bufferArea.upperLeft() to (0,0)
	util::point<int,2> translate = -tileRegion.min();
	canvas.translate(translate.x(), translate.y());

	rasterizer.draw(canvas, tileRegion);
}

void
TilesCache::cleanUp() {

	LOG_ALL(tilescachelog) << "background clean-up thread started" << std::endl;

	while (true) {

		bool haveWork = false;

		{
			// make sure we don't miss this flag
			boost::unique_lock<boost::mutex> lock(_haveDirtyTilesMutex);

			if (_haveDirtyTiles) {

				// there is something to do
				haveWork = true;
				_haveDirtyTiles = false;

			} else {

				LOG_ALL(tilescachelog) << "waiting for dirty tiles" << std::endl;

				// Now we know that there is nothing to do at the moment -- we 
				// can savely wait. This releases the lock on 
				// _haveDirtyTilesMutex, such that the producer thread can make 
				// changes to it without having to wait. After that, we will be 
				// unblocked.
				_wakeupBackgroundRasterizer.wait(lock);
			}
		}

		if (_backgroundRasterizerStopped)
			return;

		if (haveWork) {

			LOG_ALL(tilescachelog) << "cleaning dirty tiles" << std::endl;

			while (cleanDirtyTiles(2))
				if (_backgroundRasterizerStopped)
					return;

			haveWork = false;
		}
	}
}

bool
TilesCache::findInvalidTile(version_tag::version_type& mappingVersion, util::point<int,2>& tile, util::point<int,2>& physicalTile, util::box<int,2>& tileRegion) {

	// for every radius around center
	for (int radius = 0; radius < std::max((int)Width, (int)Height)/2; radius++) {

		mappingVersion = _mappingVersionTag.get_version();

		// someone is currently updating the mapping
		if (version_tag::is_locked(mappingVersion))
			return false;

		util::point<int,2> center = _mapping.get_region().center();

		LOG_ALL(tilescachelog) << "looking for dirty tiles around " << center << " with radius " << radius << std::endl;

		// TODO: check center tile only once

		// top
		for (int x = -radius + 1; x < radius; x++) {

			tile.x() = center.x() - x;
			tile.y() = center.y() - radius;

			if (isInvalid(tile, physicalTile, tileRegion))
				return true;
		}

		// right
		for (int y = -radius; y <= radius; y++) {

			tile.x() = center.x() - radius;
			tile.y() = center.y() + y;

			if (isInvalid(tile, physicalTile, tileRegion))
				return true;
		}

		// bottom
		for (int x = -radius + 1; x < radius; x++) {

			tile.x() = center.x() + x;
			tile.y() = center.y() + radius;

			if (isInvalid(tile, physicalTile, tileRegion))
				return true;
		}

		// left
		for (int y = -radius; y <= radius; y++) {

			tile.x() = center.x() + radius;
			tile.y() = center.y() - y;

			if (isInvalid(tile, physicalTile, tileRegion))
				return true;
		}
	}

	return false;
}

unsigned int
TilesCache::cleanDirtyTiles(unsigned int  maxNumRequests) {

	unsigned int cleaned = 0;

	version_tag::version_type mappingVersion;

	for (cleaned = 0; cleaned < maxNumRequests; cleaned++) {

		util::point<int,2> tile;
		util::point<int,2> physicalTile;
		util::box<int,2>  tileRegion(0, 0, 0, 0);

		if (!findInvalidTile(mappingVersion, tile, physicalTile, tileRegion))
			return cleaned;

		// the mapping changed while we were computing the physical tile and 
		// region
		if (_mappingVersionTag.changed(mappingVersion))
			return cleaned;

		LOG_DEBUG(tilescachelog) << "cleaning physical tile " << physicalTile << std::endl;

		// update it
		updateTile(physicalTile, tileRegion, *_backgroundRasterizer);

		_tileChanged[physicalTile.x()][physicalTile.y()] = true;

		// inform ohers
		if (_tileChangedCallback) {

			LOG_ALL(tilescachelog) << "invoking tile changed callback" << std::endl;
			_tileChangedCallback(tile);
		}
	}

	return cleaned;
}

bool
TilesCache::isInvalid(const util::point<int,2>& tile, util::point<int,2>& physicalTile, util::box<int,2>& tileRegion) {

	// get physical tile
	physicalTile = _mapping.map(tile);

	LOG_ALL(tilescachelog) << "probing tile " << tile << std::endl;

	if (_tileStates[physicalTile.x()][physicalTile.y()] == Invalid) {

		LOG_ALL(tilescachelog) << "tile " << tile << " is invalid" << std::endl;

		// get the region covered by the tile in pixels
		tileRegion = util::box<int,2>(tile.x(), tile.y(), tile.x() + 1, tile.y() + 1);
		tileRegion *= static_cast<int>(TileSize);

		return true;
	}

	return false;
}

