#ifndef SPECTROMETER__HXX
#define SPECTROMETER__HXX

#include <ui_spectrometer.h>
#include <libAudio.h>

class spectrometer_t final : public QMainWindow
{
	Q_OBJECT

private:
	Ui::spectrometer window;

public:
	explicit spectrometer_t(QWidget *parent = nullptr) noexcept;
	~spectrometer_t() noexcept { }
};

#endif /*SPECTROMETER__HXX*/
