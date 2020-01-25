#include <QtWidgets/QApplication>
#include "spectrometer.hxx"

int main(int argc, char **argv)
{
	QApplication app{argc, argv};
	spectrometer_t window{};

	window.show();
	return app.exec();
}

spectrometer_t::spectrometer_t(QWidget *parent) noexcept : QMainWindow{parent}, window{}
{
	window.setupUi(this);
}
