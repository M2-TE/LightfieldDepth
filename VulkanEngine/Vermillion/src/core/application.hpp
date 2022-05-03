#pragma once

class Application
{
public:
	Application() = default;
	~Application() = default;
	ROF_COPY_MOVE_DELETE(Application)

public:
	void run()
	{
		VMI_LOG("Running.");
	}

private:
	//
};