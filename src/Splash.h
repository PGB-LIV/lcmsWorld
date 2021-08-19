
class Splash
{
public:
	Splash();
	bool Render();
	static Splash* instance;
private:
	GLFWwindow* window;
	int count;
	bool finished = false;
	TimeStamp startTime;

	
};
