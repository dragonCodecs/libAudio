g++ -fPIC $(xmms-config --cflags) $(xmms-config --libs) -I../OptimFROG -O2 -W -Wall xmms_ofr.cpp ../OptimFROG/OptimFROGDecoder.cpp /usr/lib/libOptimFROG.so -shared -s -o libofr.so
