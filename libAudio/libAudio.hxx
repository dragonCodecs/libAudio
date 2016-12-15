#ifndef LIBAUDIO_HXX
#define LIBAUDIO_HXX

#include <stdint.h>
#include <memory>
#include <vector>
#include "fd.hxx"

struct fileInfo_t final
{
	uint64_t totalTime;
	uint32_t bitsPerSample;
	uint32_t bitRate;
	uint32_t channels;
	// int bitStream;
	std::unique_ptr<char []> title;
	std::unique_ptr<char []> artist;
	std::unique_ptr<char []> album;
	std::vector<std::unique_ptr<char []>> other;

	fileInfo_t() noexcept : totalTime(0), bitsPerSample(0), bitRate(0),
		channels(0), title(), artist(), album(), other() { }
};

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
	moduleFC1x = 17
};

// TODO: implement fd_t for this. It will become the new hub of IO in the library as we convert things over to this.

struct audioFile_t
{
protected:
	audioType_t _type;
	fileInfo_t _fileInfo;
	fd_t _fd;

	audioFile_t(audioType_t type, fd_t &&fd) noexcept : _type(type), _fileInfo(), _fd(std::move(fd)) { }

public:
	audioFile_t(audioFile_t &&) = default;
	virtual ~audioFile_t() noexcept { }
	audioFile_t &operator =(audioFile_t &&) = default;
	static audioFile_t *openR(const char *const fileName) noexcept;
	static bool isAudio(const char *const fileName) noexcept;
	static bool isAudio(const int fd) noexcept;
	const fileInfo_t &fileInfo() const noexcept { return _fileInfo; }
	audioType_t type() const noexcept { return _type; }

	virtual int64_t fillBuffer(void *const buffer, uint32_t length) = 0;
	virtual bool close() = 0;
	virtual void play() = 0;
	virtual void pause() = 0;
	virtual void stop() = 0;

	audioFile_t(const audioFile_t &) = delete;
	audioFile_t &operator =(const audioFile_t &) = delete;
};

struct oggVorbis_t final : public audioFile_t
{
public:
	int64_t fillBuffer(void *const buffer, uint32_t length) final override;
	bool close() final override;
	void play() final override;
	void pause() final override;
	void stop() final override;
};

struct flac_t final : public audioFile_t
{
private:
	struct decoderContext_t;
	std::unique_ptr<decoderContext_t> ctx;

	flac_t(fd_t &&fd);

public:
	static flac_t *openR(const char *const fileName) noexcept;
	static bool isFLAC(const char *const fileName) noexcept;
	static bool isFLAC(const int fd) noexcept;

	int64_t fillBuffer(void *const buffer, uint32_t length) final override;
	bool close() final override;
	void play() final override;
	void pause() final override;
	void stop() final override;
};

#endif /*LIBAUDIO_HXX*/
