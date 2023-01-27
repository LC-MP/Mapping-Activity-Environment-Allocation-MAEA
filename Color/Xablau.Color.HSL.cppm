// This is an independent project of an individual developer. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// MIT License
//
// Copyright (c) 2023 Jean Amaro <jean.amaro@outlook.com.br>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

export module xablau.color:HSL;
export import :forward_declarations;
export import :utils;

export import <algorithm>;
export import <format>;
export import <numbers>;
export import <stdexcept>;

export namespace xablau::color
{
	template < std::floating_point Type >
	class HSL final
	{
	private:
		static void check_boundaries_with_exception(const HSL &color)
		{
			if (!color.valid_boundaries())
			{
				constexpr auto minValues = HSL::min_values();
				constexpr auto maxValues = HSL::max_values();

				throw std::domain_error(
					std::format(
						"HSL colors must be {{ hue:[{:.{}f}, {:.{}f}], saturation:[{:.{}f}, {:.{}f}], lightness:[{:.{}f}, {:.{}f}] }}.",
						minValues.hue, std::numeric_limits < Type > ::max_digits10,
						maxValues.hue, std::numeric_limits < Type > ::max_digits10,
						minValues.saturation, std::numeric_limits < Type > ::max_digits10,
						maxValues.saturation, std::numeric_limits < Type > ::max_digits10,
						minValues.lightness, std::numeric_limits < Type > ::max_digits10,
						maxValues.lightness, std::numeric_limits < Type > ::max_digits10));
			}
		}

		static void check_for_NaNs_with_exception(const HSL &color)
		{
			if (std::isnan(color.hue) || std::isnan(color.saturation) || std::isnan(color.lightness))
			{
				throw std::invalid_argument("HSL cannot have NaN values.");
			}
		}

		[[nodiscard]] static Type conic_distance_to(const HSL &color1, const HSL &color2)
		{
			const auto radiusTerm1 = Type{1} - std::abs(color1.lightness - Type{0.5}) * Type{2};
			const auto radiusTerm2 = Type{1} - std::abs(color2.lightness - Type{0.5}) * Type{2};

			const auto x1 = radiusTerm1 * color1.saturation * std::cos(color1.hue);
			const auto y1 = radiusTerm1 * color1.saturation * std::sin(color1.hue);

			const auto x2 = radiusTerm2 * color2.saturation * std::cos(color2.hue);
			const auto y2 = radiusTerm2 * color2.saturation * std::sin(color2.hue);

			const auto differenceX = x2 - x1;
			const auto differenceY = y2 - y1;
			const auto differenceZ = color2.lightness - color1.lightness;

			constexpr auto maxValue = Type{2};

			return std::sqrt(differenceX * differenceX + differenceY * differenceY + differenceZ * differenceZ) / maxValue;
		}

		[[nodiscard]] static Type cylindrical_distance_to(const HSL &color1, const HSL &color2)
		{
			const auto x1 = color1.saturation * std::cos(color1.hue);
			const auto y1 = color1.saturation * std::sin(color1.hue);

			const auto x2 = color2.saturation * std::cos(color2.hue);
			const auto y2 = color2.saturation * std::sin(color2.hue);

			const auto differenceX = x2 - x1;
			const auto differenceY = y2 - y1;
			const auto differenceZ = color2.lightness - color1.lightness;

			const auto maxValue = std::sqrt(Type{5});

			return std::sqrt(differenceX * differenceX + differenceY * differenceY + differenceZ * differenceZ) / maxValue;
		}

		template < typename RGBColor, size_t BitsPerChannel >
		[[nodiscard]] static RGB < RGBColor, BitsPerChannel > convert_to_RGB(const HSL &color)
		{
			constexpr auto normalizeHue =
				[] (const Type hue) -> Type
				{
					if (hue >= Type{})
					{
						return std::fmod(hue, std::numbers::pi_v < Type >);
					}

					return std::numbers::pi_v < Type > + std::fmod(hue, std::numbers::pi_v < Type >);
				};

			const auto normalizedHue = normalizeHue(color.hue);
			const auto hueMaxValue = std::nexttoward(Type{2} * std::numbers::pi_v < Type >, Type{});
			const auto chroma = color.saturation * (Type{1} - std::abs(Type{2} * color.lightness - Type{1}));
			const auto hueLine = normalizedHue / (hueMaxValue / Type{6});
			const auto X = chroma * (Type{1} - std::abs(std::fmod(hueLine, Type{2}) - Type{1}));

			std::array < Type, 3 > auxRGB{};

			if (hueLine < Type{1})
			{
				auxRGB[0] = chroma;
				auxRGB[1] = X;
			}

			else if (hueLine < Type{2})
			{
				auxRGB[0] = X;
				auxRGB[1] = chroma;
			}

			else if (hueLine < Type{3})
			{
				auxRGB[1] = chroma;
				auxRGB[2] = X;
			}

			else if (hueLine < Type{4})
			{
				auxRGB[1] = X;
				auxRGB[2] = chroma;
			}

			else if (hueLine < Type{5})
			{
				auxRGB[0] = X;
				auxRGB[2] = chroma;
			}

			else
			{
				auxRGB[0] = chroma;
				auxRGB[2] = X;
			}

			const auto m = color.lightness - Type{0.5} * chroma;

			constexpr auto maxValuesRGB = RGB < RGBColor, BitsPerChannel > ::max_values();

			return RGB < RGBColor, BitsPerChannel >
				{
					static_cast < RGBColor > ((auxRGB[0] + m) * maxValuesRGB.red),
					static_cast < RGBColor > ((auxRGB[1] + m) * maxValuesRGB.green),
					static_cast < RGBColor > ((auxRGB[2] + m) * maxValuesRGB.blue)
				};
		}

	public:
		using value_type = Type;

		Type hue{};
		Type saturation{};
		Type lightness{};

		[[nodiscard]] static consteval HSL min_values()
		{
			return HSL{ -std::numeric_limits < Type > ::max(), Type{}, Type{} };
		}

		[[nodiscard]] static consteval HSL max_values()
		{
			return HSL{ std::numeric_limits < Type > ::max(), Type{1}, Type{1} };
		}

		[[nodiscard]] constexpr bool valid_boundaries() const
		{
			constexpr auto minValues = HSL::min_values();
			constexpr auto maxValues = HSL::max_values();

			return
				this->hue >= minValues.hue && this->saturation >= minValues.saturation && this->lightness >= minValues.lightness &&
				this->hue <= maxValues.hue && this->saturation <= maxValues.saturation && this->lightness <= maxValues.lightness;
		}

		constexpr HSL &clamp_out_of_boundaries_values()
		{
			constexpr auto minValues = HSL::min_values();
			constexpr auto maxValues = HSL::max_values();

			this->hue = std::clamp(this->hue, minValues.hue, maxValues.hue);
			this->saturation = std::clamp(this->saturation, minValues.saturation, maxValues.saturation);
			this->lightness = std::clamp(this->lightness, minValues.lightness, maxValues.lightness);

			return *this;
		}

		template < boundaries_check CheckBoundaries = boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs >
		[[nodiscard]] Type conic_distance_to(const HSL &color) const
		{
			if constexpr (CheckBoundaries == boundaries_check::check_with_exception)
			{
				HSL::check_boundaries_with_exception(*this);
				HSL::check_boundaries_with_exception(color);
			}

			else if constexpr (CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs)
			{
				HSL::check_for_NaNs_with_exception(*this);
				HSL::check_for_NaNs_with_exception(color);
			}

			if constexpr (
				CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values ||
				CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs)
			{
				auto _this = *this;
				auto _color = color;

				return HSL::conic_distance_to(_this.clamp_out_of_boundaries_values(), _color.clamp_out_of_boundaries_values());
			}

			else
			{
				return HSL::conic_distance_to(*this, color);
			}
		}

		template < boundaries_check CheckBoundaries = boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs >
		[[nodiscard]] Type cylindrical_distance_to(const HSL &color) const
		{
			if constexpr (CheckBoundaries == boundaries_check::check_with_exception)
			{
				HSL::check_boundaries_with_exception(*this);
				HSL::check_boundaries_with_exception(color);
			}

			else if constexpr (CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs)
			{
				HSL::check_for_NaNs_with_exception(*this);
				HSL::check_for_NaNs_with_exception(color);
			}

			if constexpr (
				CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values ||
				CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs)
			{
				auto _this = *this;
				auto _color = color;

				return HSL::cylindrical_distance_to(_this.clamp_out_of_boundaries_values(), _color.clamp_out_of_boundaries_values());
			}

			else
			{
				return HSL::cylindrical_distance_to(*this, color);
			}
		}

		template <
			typename RGBColor,
			size_t BitsPerChannel,
			boundaries_check CheckBoundaries = boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs >
		[[nodiscard]] RGB < RGBColor, BitsPerChannel > convert_to_RGB() const
		{
			if constexpr (CheckBoundaries == boundaries_check::check_with_exception)
			{
				HSL::check_boundaries_with_exception(*this);
			}

			else if constexpr (CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs)
			{
				HSL::check_for_NaNs_with_exception(*this);
			}

			if constexpr (
				CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values ||
				CheckBoundaries == boundaries_check::clamp_out_of_boundaries_values_and_check_for_NaNs)
			{
				auto _this = *this;

				return HSL::convert_to_RGB < RGBColor, BitsPerChannel > (_this.clamp_out_of_boundaries_values());
			}

			else
			{
				return HSL::convert_to_RGB < RGBColor, BitsPerChannel > (*this);
			}
		}
	};
}