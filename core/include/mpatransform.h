
#ifndef MPA_TRANSFORM_H
#define MPA_TRANSFORM_H

#include <Eigen/Dense>
#include <stdexcept>
#include "track.h"

namespace core {

/** \brief Performs various transforms and index translations for MPA type sensors
 *
 * The MaPSA-light geometry is rather complex…
 * on a 16x3 grid:
 *  * the pixels on the left and right have a width of 200µm
 *  * the inner pixels have a width of 100µm
 *  * the upper two rows have a pixel height of 
 *  * the third row pixels have a height of
 *
 * The correspondence between raw index and pixel coordinate is as follows:
 *     x=00 01 02 .. 14 15
 * y=3   32 33 34 .. 46 47
 * y=1   31 30 29 .. 17 16
 * y=0   00 01 02 .. 14 15
 *
 * The reference point for the world coordinates is the lower-left corner of pixel (0/0).
 *
 * \warning So far only the "light" type sensor geometry is implemented!
 */
class MpaTransform
{
public:
	static constexpr int num_pixels_x = 16;
	static constexpr int num_pixels_y = 3;
	static constexpr int num_pixels = num_pixels_x * num_pixels_y;
	static constexpr double outer_pixel_width = 0.2;
	static constexpr double inner_pixel_width = 0.1;
	static constexpr double upper_pixel_height = 1.446;
	static constexpr double bottom_pixel_height = 1.746;
	static constexpr double total_width = 2 * outer_pixel_width + 14 * inner_pixel_width; // 18mm
	static constexpr double total_height = 2 * upper_pixel_height + bottom_pixel_height;

	MpaTransform()
	{
		// Initialize normal vector and rotation matrix
		setRotation(0.0);
	}

	/// \brief Transform pixel index to world coordinates	
	Eigen::Vector3d transform(const size_t& pixelIdx, bool midpoints=true) const
	{
		return pixelCoordToGlobal(translatePixelIndex(pixelIdx), midpoints);
	}

	/// \brief Translates the pixel index to 2D pixel coordinates
	Eigen::Vector2i translatePixelIndex(const size_t& pixelIdx) const
	{
		if(pixelIdx > num_pixels) {
			throw std::out_of_range("Pixel index out of range");
		}
		int py = 2 - pixelIdx/16;
		return Eigen::Vector2i(
			((py==1)?15:0) + static_cast<unsigned int>(pixelIdx)%16 * ((py==1)?-1:1),
			py
		);
	}

	/** \brief Transforms pixel coordinates to world-space coordinates
	 *
	 * Takes geometry and positioning of the MPA into account.
	 */
	Eigen::Vector3d pixelCoordToGlobal(const Eigen::Vector2i& pixelCoord, bool midpoints=true) const
	{
		Eigen::Vector2d newCoord{pixelCoord.cast<double>()};
		if(midpoints) {
			newCoord += Eigen::Vector2d(0.5, 0.5);
		}
		return pixelCoordToGlobal(newCoord);
	}

	/** \brief Transforms pixel coordinates to world-space coordinates
	 *
	 * Takes geometry and positioning of the MPA into account. This is the generalized version which takes
	 * floating-point coordinates.
	 *
	 * \warning The distance metric is not the same between the pixels, though it is uniform on a single pixel.
	 * This means a distance of "0.1 pixel coordinates" is different between an outer and inner pixel due
	 * to different pixel geometries!
	 */
	Eigen::Vector3d pixelCoordToGlobal(const Eigen::Vector2d& pixelCoord) const
	{
		// Express pixel coordinates in "module coordinates" with uniform scale across all pixels
		// X coordinate simple because outer pixels just have double width :)
		double x = pixelCoord(0) + 1.0;
		if(pixelCoord(0) >= 15.0) {
			x = (pixelCoord(0)-15.0)*2 + 16.0;
		} else if(pixelCoord(0) < 1) {
			x = pixelCoord(0)*2;
		}
		x /= 18;
		double y = pixelCoord(1);
		const double bottom_scale = bottom_pixel_height/upper_pixel_height;
		if(y >= 2.0) {
			y = (pixelCoord(1)-2) * bottom_scale + 2.0;
		}
		y /= (2 + bottom_scale);
		// We now have the "module coordinates" x and y in a [0:1] range
		Eigen::Vector3d coord(x*total_width, y*total_height, 0.0);
		// todo: rotate
		coord += _offset;
		return coord;
	}

	Eigen::Vector3d mpaPlaneTrackIntersect(const Track& track, const size_t& a=0, const size_t& b=1) const
	{
		Eigen::Hyperplane<double, 3> plane(_normal, _offset);
		Eigen::ParametrizedLine<double, 3> line(track.points.at(a),
		                                     track.points.at(b) - track.points.at(a));
		auto t = line.intersectionParameter(plane);
		return line.pointAt(t);
	}

	size_t getPixelIndex(const Eigen::Vector3d& global) const
	{
		return pixelCoordToIndex(globalToPixelCoord(global).cast<int>());
	}

	size_t getPixelIndex(const Track& track, const size_t& a=0, const size_t& b=5) const
	{
		return pixelCoordToIndex(globalToPixelCoord(mpaPlaneTrackIntersect(track, a, b)).cast<int>());
	}

	size_t pixelCoordToIndex(const Eigen::Vector2i& local) const
	{
		if((local.array() < 0 || local.array() >= Eigen::Array2i(num_pixels_x, num_pixels_y)).any()) {
			throw std::out_of_range("Hit is not in pixel plane");
		}
		if(local(1) == 1) {
			return 31 - local(0);
		} else if(local(1) == 0) {
			return 32 + local(0);
		} else if(local(1) == 2) {
			return local(0);
		}
		return 0; // todo implement further pixel arrangements...
	}

	Eigen::Vector2d globalToPixelCoord(const Eigen::Vector3d& global) const
	{
		auto local = global - _offset;
		// todo: un-rotate
		const double bottom_scale = bottom_pixel_height / upper_pixel_height;
		// scale the local coordinates to module coordinates ([0,1] range across sensor)
		// and then apply the "virtual pixel count"
		//  * 18 in X directoin (outer pixel double counted)
		//  * 2+k in Y direction
		double module_x = local(0)/total_width*18;
		double module_y = local(1)/total_height*(2+bottom_scale);
		double pixel_x = module_x - 1.0;
		if(module_x < 2.0) {
			pixel_x = module_x / 2.0;
		} else if (module_x > 16.0) {
			pixel_x = (module_x-16)/2 + 15.0;
		}
		double pixel_y = module_y;
		if(module_y > 2.0) {
			pixel_y = (module_y - 2)/bottom_scale + 2.0;
		}
		return {pixel_x, pixel_y};
	}

	void setRotation(const double& tilt)
	{
		_rotation = Eigen::AngleAxis<double>(tilt, Eigen::Vector3d::UnitX());
		_normal = _rotation * Eigen::Vector3d::UnitZ();
	}

	void setOffset(const Eigen::Vector3d& offset)
	{
		_offset = offset;
	}
	Eigen::Vector3d getOffset() const { return _offset; }

	Eigen::Vector2d getPixelSize(const size_t& idx) const { return getPixelSize(translatePixelIndex(idx)); }
	Eigen::Vector2d getPixelSize(const Eigen::Vector2i& pixel_coord) const
	{
		Eigen::Vector2d size(inner_pixel_width, upper_pixel_height);
		if(pixel_coord(0) == 0 || pixel_coord(0) == 15) {
			size(0) = outer_pixel_width;
		}
		if(pixel_coord(1) == 2) {
			size(1) = bottom_pixel_height;
		}
		return size;
	}

private:
	Eigen::Matrix3d _rotation;
	Eigen::Vector3d _normal;
	Eigen::Vector3d _offset;
};

} // namespace core

#endif
