#include <utility>

inline const char *alErrorString(const ALenum error) noexcept
{
	switch (error)
	{
		case AL_NO_ERROR:
			return "No error";
		case AL_INVALID_NAME:
			return "Invalid parameter name given";
		case AL_INVALID_ENUM:
			return "Invalid enumeration value";
		case AL_INVALID_VALUE:
			return "Invalid value - out of range";
		case AL_INVALID_OPERATION:
			return "Invalid operation";
		case AL_OUT_OF_MEMORY:
			return "Out of memory";
		default:
			return "Unknown";
	}
}

using std::declval;

template<typename function_t> struct alCall_t
{
	function_t function;
	const char *const functionName;

	template<typename... args_t> void operator ()(args_t &&... args) const noexcept
	{
		function(std::forward<args_t>(args)...);
		const auto error = alGetError();
		if (error != AL_NO_ERROR)
			fprintf(stderr, "libAudio: %s() - %s\n", functionName, alErrorString(error));
	}
};

template<typename function_t> struct alcCall_t
{
	function_t function;
	const char *const functionName;

	template<typename... args_t, typename result_t = decltype(declval<function_t>()(declval<args_t>()...)),
		typename = typename std::enable_if<!std::is_same<result_t, void>::value>::type>
		auto operator ()(args_t &&... args) const noexcept -> result_t
	{
		auto result = function(std::forward<args_t>(args)...);
		const auto error = alcGetError(nullptr);
		if (error != AL_NO_ERROR)
			fprintf(stderr, "libAudio: %s() - %s\n", functionName, alErrorString(error));
		return result;
	}

	template<typename... args_t, typename result_t = decltype(declval<function_t>()(declval<args_t>()...)),
		typename = typename std::enable_if<std::is_same<result_t, void>::value>::type>
		void operator ()(args_t &&... args) const noexcept
	{
		function(std::forward<args_t>(args)...);
		const auto error = alcGetError(nullptr);
		if (error != AL_NO_ERROR)
			fprintf(stderr, "libAudio: %s() - %s\n", functionName, alErrorString(error));
	}
};

#define AL_CALL(function) alCall_t<decltype((function))>{function, #function}
#define ALC_CALL(function) alcCall_t<decltype((function))>{function, #function}

namespace al
{
	auto alcGetString = ALC_CALL(::alcGetString);
	auto alcOpenDevice = ALC_CALL(::alcOpenDevice);
	auto alcCreateContext = ALC_CALL(::alcCreateContext);
	auto alcGetCurrentContext = ALC_CALL(::alcGetCurrentContext);
	auto alcMakeContextCurrent = ALC_CALL(::alcMakeContextCurrent);
	auto alcDestroyContext = ALC_CALL(::alcDestroyContext);
	auto alcCloseDevice = ALC_CALL(::alcCloseDevice);
	auto alGenSources = AL_CALL(::alGenSources);
	auto alSourcef = AL_CALL(::alSourcef);
	auto alSource3f = AL_CALL(::alSource3f);
	auto alListener3f = AL_CALL(::alListener3f);
	auto alDeleteSources = AL_CALL(::alDeleteSources);
	auto alSourceQueueBuffers = AL_CALL(::alSourceQueueBuffers);
	auto alSourceUnqueueBuffers = AL_CALL(::alSourceUnqueueBuffers);
	auto alSourcePlay = AL_CALL(::alSourcePlay);
	auto alSourcePause = AL_CALL(::alSourcePause);
	auto alSourceStop = AL_CALL(::alSourceStop);
	auto alGetSourcei = AL_CALL(::alGetSourcei);
	auto alGenBuffers = AL_CALL(::alGenBuffers);
	auto alDeleteBuffers = AL_CALL(::alDeleteBuffers);
	auto alBufferData = AL_CALL(::alBufferData);
}

#undef ALC_CALL
#undef AL_CALL
