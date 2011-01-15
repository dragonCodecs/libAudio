class AboutBox
{
private:
	GTKAboutDialog *About;

public:
	AboutBox(GTKWindow *Parent);
	~AboutBox();
	void Run();
};