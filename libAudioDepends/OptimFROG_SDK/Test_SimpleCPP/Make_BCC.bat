rem coff2omf ..\OptimFROG\OptimFROG.lib ..\OptimFROG\OptimFROGB.lib
bcc32 -I..\OptimFROG -O2 -w OFROGDecSimCPP.cpp ..\OptimFROG\OptimFROGDecoder.cpp ..\OptimFROG\OptimFROGB.lib
