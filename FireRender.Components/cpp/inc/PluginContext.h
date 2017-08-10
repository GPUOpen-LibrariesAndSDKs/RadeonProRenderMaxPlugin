#pragma once

class PluginContext
{
private:
	PluginContext();

public:
	PluginContext(const PluginContext&) = delete;
	PluginContext& operator=(const PluginContext&) = delete;

	static PluginContext& instance();

	bool HasSSE41() const;

	int GetNumberOfThreadsAvailableForAsyncCalls(void) const;

private:
	bool CheckSSE41();
	int GetNumberOfCores(void);

private:
	bool mHasSSE41 = false;
	int mNumOfCores = 0;
};
