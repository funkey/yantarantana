#ifndef YANTARANTANA_GUI_TEXTURE_PYRAMID_H__
#define YANTARANTANA_GUI_TEXTURE_PYRAMID_H__

#include <boost/multi_array.hpp>
#include <sg_gui/Texture.h>
#include <util/torus_mapping.hpp>
#include "Rasterizer.h"

class TexturePyramid {

public:

	static const unsigned int TileWidth  = 64;
	static const unsigned int TileHeight = 64;

	TexturePyramid();

	void render(const util::box<float, 2>& roi, Rasterizer& rasterizer);

private:

	class Tile : public sg_gui::Texture {

	public:

		Tile() : sg_gui::Texture(TexturePyramid::TileWidth, TexturePyramid::TileHeight, GL_RGBA) {}
	};

	/**
	 * Get the integer coordinates of the tile that contains the given world
	 * coordinate.
	 */
	inline util::point<int, 2> getTileCoordinates(const util::point<float, 2>& worldCoordinates) {

		return util::point<int, 2>(
				worldCoordinates.x()/static_cast<int>(TileWidth)  - (worldCoordinates.x() < 0 ? 1 : 0),
				worldCoordinates.y()/static_cast<int>(TileHeight) - (worldCoordinates.y() < 0 ? 1 : 0));
	}

	/**
	 * Get the integer roi of tiles containing the given roi in world coordinates.
	 */
	util::box<int, 2> getTiles(const util::box<float, 2>& roi);

	/**
	 * Redraw the tiles.
	 */
	void updateTiles(const util::box<float, 2>& roi, Rasterizer& rasterizer);

	util::point<int, 2> _numTiles;

	// mapping from tile coordinates to tile indices
	torus_mapping<int> _tileMapping;

	// the tiles
	Tile _tiles[100][100];
};

#endif // YANTARANTANA_GUI_TEXTURE_PYRAMID_H__

