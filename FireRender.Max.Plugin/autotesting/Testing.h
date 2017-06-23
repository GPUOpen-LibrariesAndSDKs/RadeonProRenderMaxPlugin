/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* Automatic testing tool interface
*********************************************************************************************************************************/
#pragma once
#include "Common.h"
FIRERENDER_NAMESPACE_BEGIN;

/// Implementation of the automatic testing (separated from the 3dsmax plugin logic)
class Tester {

    /// If true, the user requested the session to stop
    bool cancelled;

    /// Current progress message (such as number of tests already completed). It is automatically updated during each testing 
    /// session, so it can be periodically queried to get progress info
    std::string progressMsg;

public:

    /// Runs the tests in given directory (all sub-folders not starting with a "." (dot) will be considered a test case. Blocks
    /// until it is finished
    /// \param directory where to look for tests (sub-folders)
    /// \param filter if non-empty, only tests whose name contains the filter will be processed
    /// \param passes how many passes to render for each scene.
    void renderTests(std::string directory, std::string filter, const int passes);

    /// Stops the current testing session ASAP. Does NOT block until the session is stopped.
    void stopRender();

    std::string progress() const {
        return this->progressMsg;
    }

};


FIRERENDER_NAMESPACE_END;