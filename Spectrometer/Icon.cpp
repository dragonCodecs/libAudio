#ifdef _WINDOWS
#include <windows.h>
#endif
#include <GTK++.h>
#include "Icon.h"

#include "ico/Spectrometer_16x16.h"
#include "ico/Spectrometer_32x32.h"
#include "ico/Spectrometer_48x48.h"
#include "ico/Spectrometer_64x64.h"
#include "ico/Spectrometer_128x128.h"

GList *Icons = NULL;

void Icon::SetIcons()
{
	if (Icons == NULL)
	{
		GDKPixbuf *buff;
		buff = new GDKPixbuf(Spectrometer_16x16);
		Icons = g_list_prepend(Icons, (void *)buff->GetBuffer());
		buff = new GDKPixbuf(Spectrometer_32x32);
		Icons = g_list_prepend(Icons, (void *)buff->GetBuffer());
		buff = new GDKPixbuf(Spectrometer_48x48);
		Icons = g_list_prepend(Icons, (void *)buff->GetBuffer());
		buff = new GDKPixbuf(Spectrometer_64x64);
		Icons = g_list_prepend(Icons, (void *)buff->GetBuffer());
		buff = new GDKPixbuf(Spectrometer_128x128);
		Icons = g_list_prepend(Icons, (void *)buff->GetBuffer());
		buff = NULL;
	}
	GTK::SetDefaultWindowIcon(Icons);
}
