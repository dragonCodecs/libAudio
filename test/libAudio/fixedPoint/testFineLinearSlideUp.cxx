#include "slideTestCommon.hxx"

/* The following function generates this */
const uint32_t linearSlideUpTable[16] =
{
	65536, 65595, 65654, 65714,	65773, 65832, 65892, 65951,
	66011, 66071, 66130, 66190, 66250, 66309, 66369, 66429
};

class fineLinearSlideUp_t final : public testsuit
{
private:
	uint32_t linearSlideUp(uint8_t slide) const noexcept
	{
		constexpr fixed64_t c4{4};
		constexpr fixed64_t c192{192};
		constexpr fixed64_t c65536{65536};
		const fixed64_t result = ((fixed64_t{slide} / c4) / c192).pow2() * c65536;
		return int{result};
	}

	void testComputation()
	{
		for (uint8_t i{0}; i < 16; ++i)
		{
			const uint32_t slide = linearSlideUp(i);
			if (slide != linearSlideUpTable[i])
			{
				if (unsign(linearSlideUpTable[i] - slide) < 2)
					displayWarning(slide, linearSlideUpTable[i]);
				else
					assertEqual(slide, linearSlideUpTable[i]);
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
	{ registerTestClasses<fineLinearSlideUp_t>(); }