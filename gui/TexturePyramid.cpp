#include "TexturePyramid.h"

logger::LogChannel texturepyramidlog("texturepyramidlog", "[TexturePyramid] ");

TexturePyramid::TexturePyramid() :
	_numTiles(100, 100),
	_tileMapping(_numTiles.x(), _numTiles.y()) {}

void
TexturePyramid::render(const util::box<float,2>& roi, Rasterizer& rasterizer) {

	updateTiles(roi, rasterizer);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	util::box<int, 2> tiles = getTiles(roi);

	for (int x = tiles.min().x(); x < tiles.max().x(); x++)
		for (int y = tiles.min().y(); y < tiles.max().y(); y++) {

			LOG_ALL(texturepyramidlog) << "rendering tile " << x << ", " << y << std::endl;

			util::point<int, 2> index = _tileMapping.map(util::point<int, 2>(x, y));
			_tiles[index.x()][index.y()].bind();

			// tile position in world coordinates
			util::box<double, 2> tilePosition(
					 x   *TileWidth,  y   *TileHeight,
					(x+1)*TileWidth, (y+1)*TileHeight);

			glBegin(GL_QUADS);
			glTexCoord2d(0, 0); glVertex2d(tilePosition.min().x(), tilePosition.min().y());
			glTexCoord2d(1, 0); glVertex2d(tilePosition.max().x(), tilePosition.min().y());
			glTexCoord2d(1, 1); glVertex2d(tilePosition.max().x(), tilePosition.max().y());
			glTexCoord2d(0, 1); glVertex2d(tilePosition.min().x(), tilePosition.max().y());
			glEnd();

			_tiles[index.x()][index.y()].unbind();
		}

	glDisable(GL_BLEND);
}

util::box<int,2>
TexturePyramid::getTiles(const util::box<float,2>& roi) {

	util::box<int,2> tiles;

	tiles.min() = getTileCoordinates(roi.min());
	tiles.max() = getTileCoordinates(roi.max()) + util::point<int,2>(1, 1);

	return tiles;
}

void
TexturePyramid::updateTiles(const util::box<float,2>& roi, Rasterizer& rasterizer) {

	util::box<int, 2> tiles = getTiles(roi);

	for (int x = tiles.min().x(); x < tiles.max().x(); x++)
		for (int y = tiles.min().y(); y < tiles.max().y(); y++) {

			LOG_ALL(texturepyramidlog) << "updating tile " << x << ", " << y << std::endl;

			util::point<int, 2> index = _tileMapping.map(util::point<int, 2>(x, y));

			// TODO: use rasterizer
			sg_gui::skia_pixel_t data[TileWidth*TileHeight];
			for (unsigned int i = 0; i < TileWidth*TileHeight; i++)
				data[i] = sg_gui::skia_pixel_t(x%255, y%255, 0, 255);

			_tiles[index.x()][index.y()].loadData(data, util::box<int,2>(0, 0, TileWidth, TileHeight));
		}
}
