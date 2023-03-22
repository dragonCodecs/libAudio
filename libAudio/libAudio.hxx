// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2016-2023 Rachel Mant <git@dragonmux.network>
#ifndef LIBAUDIO_HXX
#define LIBAUDIO_HXX

/*!
 * @file libAudio.hxx
 * @brief C++ API header for use by external programs
 * @author Rachel Mant <git@dragonmux.network>
 * @date 2016-2020
 */

#include <cstring>
#ifdef _WINDOWS
#define strncasecmp strnicmp
#define strcasecmp stricmp
#endif

#include <cstdint>
#include <substrate/fd>
#include "fileInfo.hxx"
#include "playback.hxx"
#include "libAudio.h"

#if __has_cpp_attribute(nodiscard) || __cplusplus >= 201402L
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#	define libAUDIO_NO_DISCARD(x) [[nodiscard]] x
#elif defined(__GNUC__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#	define libAUDIO_NO_DISCARD(x) x __attribute__((warn_unused_result))
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#	define libAUDIO_NO_DISCARD(x) x
#endif

using substrate::fd_t;

enum class audioType_t : uint8_t
{
	oggVorbis = 1,
	flac = 2,
	wave = 3,
	m4a = 4,
	aac = 5,
	mp3 = 6,
	moduleIT = 7,
	musePack = 8,
	wavPack = 9,
	optimFROG = 10,
	realAudio = 11,
	wma = 12,
	moduleMOD = 13,
	moduleS3M = 14,
	moduleSTM = 15,
	moduleAON = 16,
	moduleFC1x = 17,
	oggOpus = 18,
	sndh = 19,
	sid = 20
};

using fileIs_t = bool (*)(const char *);
using fileOpenR_t = void *(*)(const char *);
using fileOpenW_t = void *(*)(const char *);

const fileInfo_t *audioFileInfo(void *audioFile);
bool audioFileInfo(void *audioFile, const fileInfo_t *fileInfo);
libAUDIO_CXX_API std::vector<std::string> audioOutputDevices();
libAUDIO_CXX_API bool audioDefaultDevice(const std::string &device) noexcept;
libAUDIO_CXX_API const std::string &audioDefaultDevice() noexcept;

struct audioModeRead_t { };
struct audioModeWrite_t { };

struct libAUDIO_CLSMAYBE_API audioFile_t
{
protected:
// NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
	audioType_t _type{};
	fileInfo_t _fileInfo{};
	fd_t _fd{};
	std::unique_ptr<playback_t> _player{};
// NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)

	audioFile_t(audioType_t type, fd_t &&fd) noexcept : _type{type}, _fd{std::move(fd)} { }

public:
	audioFile_t(audioFile_t &&) = default;
	virtual ~audioFile_t() noexcept = default;
	audioFile_t &operator =(audioFile_t &&) = default;
	static audioFile_t *openR(const char *fileName) noexcept;
	static audioFile_t *openW(const char *fileName) noexcept;
	static bool isAudio(const char *fileName) noexcept;
	static bool isAudio(int32_t fd) noexcept;
	const fileInfo_t &fileInfo() const noexcept { return _fileInfo; }
	fileInfo_t &fileInfo() noexcept { return _fileInfo; }
	audioType_t type() const noexcept { return _type; }
	const fd_t &fd() const noexcept { return _fd; }
	void fd(fd_t &&fd) noexcept { _fd.swap(fd); }
	void player(std::unique_ptr<playback_t> &&player) noexcept { _player = std::move(player); }

	libAUDIO_CLS_API virtual int64_t fillBuffer(void *buffer, uint32_t length) = 0;
	libAUDIO_CLS_API virtual int64_t writeBuffer(const void *buffer, int64_t length);
	libAUDIO_CLS_API virtual bool fileInfo(const fileInfo_t &fileInfo);
	libAUDIO_CLS_API bool playbackMode(playbackMode_t mode) noexcept;
	libAUDIO_CLS_API void playbackVolume(float level) noexcept;
	libAUDIO_CLS_API void play();
	libAUDIO_CLS_API void pause();
	libAUDIO_CLS_API void stop();

	audioFile_t(const audioFile_t &) = delete;
	audioFile_t &operator =(const audioFile_t &) = delete;
};

#ifdef ENABLE_VORBIS
struct oggVorbis_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	struct encoderContext_t;
	std::unique_ptr<decoderContext_t> decoderCtx;
	std::unique_ptr<encoderContext_t> encoderCtx;

public:
	oggVorbis_t(fd_t &&fd, audioModeRead_t) noexcept;
	oggVorbis_t(fd_t &&fd, audioModeWrite_t) noexcept;
	static oggVorbis_t *openR(const char *fileName) noexcept;
	static oggVorbis_t *openW(const char *fileName) noexcept;
	static bool isOggVorbis(const char *fileName) noexcept;
	static bool isOggVorbis(int32_t fd) noexcept;
	decoderContext_t *decoderContext() const noexcept { return decoderCtx.get(); }
	encoderContext_t *encoderContext() const noexcept { return encoderCtx.get(); }
	bool valid() const noexcept { return (bool(decoderCtx) || bool(encoderCtx)) && _fd.valid(); }

	using audioFile_t::fileInfo;
	int64_t fillBuffer(void *buffer, uint32_t length) final;
	int64_t writeBuffer(const void *buffer, int64_t length) final;
	bool fileInfo(const fileInfo_t &fileInfo) final;
};
#endif // ENABLE_VORBIS

#ifdef ENABLE_OPUS
struct oggOpus_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	struct encoderContext_t;
	std::unique_ptr<decoderContext_t> decoderCtx;
	std::unique_ptr<encoderContext_t> encoderCtx;

public:
	oggOpus_t(fd_t &&fd, audioModeRead_t) noexcept;
	oggOpus_t(fd_t &&fd, audioModeWrite_t) noexcept;
	static oggOpus_t *openR(const char *fileName) noexcept;
	static oggOpus_t *openW(const char *fileName) noexcept;
	static bool isOggOpus(const char *fileName) noexcept;
	static bool isOggOpus(int32_t fd) noexcept;
	decoderContext_t *decoderContext() const noexcept { return decoderCtx.get(); }
	encoderContext_t *encoderContext() const noexcept { return encoderCtx.get(); }
	bool valid() const noexcept { return (bool(decoderCtx) || bool(encoderCtx)) && _fd.valid(); }

	using audioFile_t::fileInfo;
	int64_t fillBuffer(void *buffer, uint32_t length) final;
	int64_t writeBuffer(const void *buffer, int64_t length) final;
	bool fileInfo(const fileInfo_t &fileInfo) final;
};
#endif // ENABLE_OPUS

#ifdef ENABLE_FLAC
struct flac_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	struct encoderContext_t;
	std::unique_ptr<decoderContext_t> decoderCtx;
	std::unique_ptr<encoderContext_t> encoderCtx;

public:
	flac_t(fd_t &&fd, audioModeRead_t) noexcept;
	flac_t(fd_t &&fd, audioModeWrite_t) noexcept;
	static flac_t *openR(const char *fileName) noexcept;
	static flac_t *openW(const char *fileName) noexcept;
	static bool isFLAC(const char *fileName) noexcept;
	static bool isFLAC(int32_t fd) noexcept;
	decoderContext_t *decoderContext() const noexcept { return decoderCtx.get(); }
	encoderContext_t *encoderContext() const noexcept { return encoderCtx.get(); }
	bool valid() const noexcept { return (bool(decoderCtx) || bool(encoderCtx)) && _fd.valid(); }

	using audioFile_t::fileInfo;
	int64_t fillBuffer(void *buffer, uint32_t length) final;
	int64_t writeBuffer(const void *buffer, int64_t length) final;
	bool fileInfo(const fileInfo_t &fileInfo) final;
};
#endif // ENABLE_FLAC

struct wav_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	bool skipToChunk(const std::array<char, 4> &chunkName) const noexcept;
	bool readFormat() noexcept;

public:
	wav_t() noexcept;
	wav_t(fd_t &&fd) noexcept;
	static wav_t *openR(const char *fileName) noexcept;
	static bool isWAV(const char *fileName) noexcept;
	static bool isWAV(int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};

#ifdef ENABLE_M4A
struct m4a_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	struct encoderContext_t;
	std::unique_ptr<decoderContext_t> decoderCtx;
	std::unique_ptr<encoderContext_t> encoderCtx;

public:
	m4a_t(fd_t &&fd, audioModeRead_t) noexcept;
	m4a_t(fd_t &&fd, audioModeWrite_t) noexcept;
	static m4a_t *openR(const char *fileName) noexcept;
	static m4a_t *openW(const char *fileName) noexcept;
	static bool isM4A(const char *fileName) noexcept;
	static bool isM4A(int32_t fd) noexcept;
	decoderContext_t *decoderContext() const noexcept { return decoderCtx.get(); }
	encoderContext_t *encoderContext() const noexcept { return encoderCtx.get(); }
	bool valid() const noexcept { return (bool(decoderCtx) || bool(encoderCtx)) && _fd.valid(); }

	using audioFile_t::fileInfo;
	int64_t fillBuffer(void *buffer, uint32_t length) final;
	int64_t writeBuffer(const void *buffer, int64_t length) final;
	bool fileInfo(const fileInfo_t &fileInfo) final;
	void fetchTags() noexcept;
};

struct aac_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	uint8_t *nextFrame() noexcept;

public:
	aac_t(fd_t &&fd) noexcept;
	static aac_t *openR(const char *fileName) noexcept;
	static bool isAAC(const char *fileName) noexcept;
	static bool isAAC(int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};
#endif // ENABLE_AAC

#ifdef ENABLE_MP3
struct mp3_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	struct encoderContext_t;
	std::unique_ptr<decoderContext_t> decoderCtx;
	std::unique_ptr<encoderContext_t> encoderCtx;

	libAUDIO_NO_DISCARD(bool readMetadata() noexcept);

public:
	mp3_t(fd_t &&fd, audioModeRead_t) noexcept;
	mp3_t(fd_t &&fd, audioModeWrite_t) noexcept;
	static mp3_t *openR(const char *fileName) noexcept;
	static mp3_t *openW(const char *fileName) noexcept;
	static bool isMP3(const char * fileName) noexcept;
	static bool isMP3(int32_t fd) noexcept;
	decoderContext_t *decoderContext() const noexcept { return decoderCtx.get(); }
	encoderContext_t *encoderContext() const noexcept { return encoderCtx.get(); }
	bool valid() const noexcept { return (bool(decoderCtx) || bool(encoderCtx)) && _fd.valid(); }

	using audioFile_t::fileInfo;
	int64_t fillBuffer(void *buffer, uint32_t length) final;
	int64_t writeBuffer(const void *buffer, int64_t length) final;
	bool fileInfo(const fileInfo_t &fileInfo) final;
};
#endif // ENABLE_MP3

struct moduleFile_t : public audioFile_t
{
protected:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	moduleFile_t(audioType_t type, fd_t &&fd) noexcept;

public:
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};

struct modMOD_t final : public moduleFile_t
{
public:
	modMOD_t(fd_t &&fd) noexcept;
	static modMOD_t *openR(const char *fileName) noexcept;
	static bool isMOD(const char *fileName) noexcept;
	static bool isMOD(int32_t fd) noexcept;
};

struct modS3M_t final : public moduleFile_t
{
public:
	modS3M_t(fd_t &&fd) noexcept;
	static modS3M_t *openR(const char *fileName) noexcept;
	static bool isS3M(const char *fileName) noexcept;
	static bool isS3M(int32_t fd) noexcept;
};

struct modSTM_t final : public moduleFile_t
{
public:
	modSTM_t(fd_t &&fd) noexcept;
	static modSTM_t *openR(const char *fileName) noexcept;
	static bool isSTM(const char *fileName) noexcept;
	static bool isSTM(int32_t fd) noexcept;
};

struct modIT_t final : public moduleFile_t
{
public:
	modIT_t(fd_t &&fd) noexcept;
	static modIT_t *openR(const char *fileName) noexcept;
	static bool isIT(const char *fileName) noexcept;
	static bool isIT(int32_t fd) noexcept;
};

#ifdef ENABLE_AON
struct modAON_t final : public moduleFile_t
{
public:
	modAON_t() noexcept;
	modAON_t(fd_t &&fd) noexcept;
	static modAON_t *openR(const char *fileName) noexcept;
	static bool isAON(const char *fileName) noexcept;
	static bool isAON(int32_t fd) noexcept;
};
#endif

#ifdef ENABLE_FC1x
struct modFC1x_t final : public moduleFile_t
{
public:
	modFC1x_t() noexcept;
	modFC1x_t(fd_t &&fd) noexcept;
	static modFC1x_t *openR(const char *fileName) noexcept;
	static bool isFC1x(const char *fileName) noexcept;
	static bool isFC1x(int32_t fd) noexcept;
};
#endif

#ifdef ENABLE_MUSEPACK
struct mpc_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	mpc_t(fd_t &&fd) noexcept;
	static mpc_t *openR(const char *fileName) noexcept;
	static bool isMPC(const char *fileName) noexcept;
	static bool isMPC(int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};
#endif // ENABLE_MUSEPACK

#ifdef ENABLE_WAVPACK
struct wavPack_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	wavPack_t(fd_t &&fd, const char *const fileName) noexcept;
	static wavPack_t *openR(const char *fileName) noexcept;
	static bool isWavPack(const char *fileName) noexcept;
	static bool isWavPack(int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};
#endif // ENABLE_WAVPACK

#ifdef ENABLE_SNDH
struct sndh_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	sndh_t(fd_t &&fd) noexcept;
	static sndh_t *openR(const char *fileName) noexcept;
	static bool isSNDH(const char *fileName) noexcept;
	static bool isSNDH(int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};
#endif

#ifdef ENABLE_SID
struct sid_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	sid_t(fd_t &&fd) noexcept;
	static sid_t *openR(const char *fileName) noexcept;
	static bool isSID(const char *fileName) noexcept;
	static bool isSID(int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};
#endif // ENABLE_SID

#ifdef ENABLE_OptimFROG
struct optimFROG_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

public:
	optimFROG_t(fd_t &&fd) noexcept;
	static optimFROG_t *openR(const char *fileName) noexcept;
	static bool isOptimFROG(const char *fileName) noexcept;
	static bool isOptimFROG(int32_t fd) noexcept;
	decoderContext_t *context() const noexcept { return ctx.get(); }
	bool valid() const noexcept { return bool(ctx) && _fd.valid(); }

	int64_t fillBuffer(void *buffer, uint32_t length) final;
};
#endif // ENABLE_OptimFROG

#endif /*LIBAUDIO_HXX*/
