rem coff2omf ..\OptimFROG\OptimFROG.lib ..\OptimFROG\OptimFROGB.lib
bcc32 -I..\OptimFROG -O2 -w OFROGDecSim.c ..\OptimFROG\OptimFROGB.lib
