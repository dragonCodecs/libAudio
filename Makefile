default: all

all:
	@cd libAudio && $(MAKE) all
	@cd libAudio && sudo $(MAKE) install
	@cd libAudio_TestApp && $(MAKE) all
	@cd libAudio_TestApp && $(MAKE) install

clean:
	@cd libAudio && $(MAKE) clean
	@cd libAudio_TestApp && $(MAKE) clean
	rm -f *~
