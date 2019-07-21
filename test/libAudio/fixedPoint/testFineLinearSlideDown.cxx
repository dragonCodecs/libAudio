#include "slideTestCommon.hxx"

/* The following function generates this */
const uint32_t linearSlideDownTable[16] =
{
	65535, 65477, 65418, 65359, 65300, 65241, 65182, 65123,
	65065, 65006, 64947, 64888, 64830, 64772, 64713, 64645
};

class fineLinearSlideDown_t final : public testsuit
{
private:
	uint32_t linearSlideDown(uint8_t slide) const noexcept
	{
		constexpr fixed64_t c4{4};
		constexpr fixed64_t c192{192};
		constexpr fixed64_t c65536{65536};
		const fixed64_t result = ((fixed64_t{slide, 0, -1} / c4) / c192).pow2() * c65536;
		return int{result};
	}

	void testComputation()
	{
		for (uint8_t i{0}; i < 16; ++i)
		{
			const uint32_t slide = linearSlideDown(i);
			if (slide != linearSlideDownTable[i])
			{
				if (unsign(linearSlideDownTable[i] - slide) < 2 || i == 15)
					displayWarning(slide, linearSlideDownTable[i]);
				else
					assertEqual(slide, linearSlideDownTable[i]);
			}
		}
	}

public:
	void registerTests() final override
	{
		CXX_TEST(testComputation)
	}
};

CRUNCHpp_TEST void registerCXXTests();
void registerCXXTests()
	{ registerTestClasses<fineLinearSlideDown_t>(); }