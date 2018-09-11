include Makefile.inc

default: all

all:
	@cd libAudio && $(MAKE) all
	@cd libAudio && $(MAKE) install
	@cd player && $(MAKE) all
	@cd player && $(MAKE) install
	@cd libAudio_ReEncode_Ogg && $(MAKE) all
	@cd libAudio_ReEncode_Ogg && $(MAKE) install
#	@cd libAudio_S3MHasCmd && $(MAKE) all
#	@cd libAudio_S3MHasCmd && $(MAKE) install

clean:
	@cd libAudio && $(MAKE) clean
	@cd player && $(MAKE) clean
	@cd libAudio_ReEncode_Ogg && $(MAKE) clean
	@cd libAudio_S3MHasCmd && $(MAKE) clean
