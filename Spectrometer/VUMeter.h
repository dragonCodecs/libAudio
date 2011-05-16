class VUMeter
{
private:
	double value;
	GTKVBox *Widget;
	GTKImage *Bars[40];
	GDKPixbuf *Black, *Red, *Amber, *Green;

public:
	VUMeter();
	~VUMeter();
	void SetValue(uint32_t Value);
	void ResetMeter();
	GTKVBox *GetWidget();
};
