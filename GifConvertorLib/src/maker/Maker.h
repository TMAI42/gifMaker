#pragma once

#include "..\Deleters\Deleters.h"
#include <memory>

class Maker {
public:
    virtual void AddFrame(std::unique_ptr<AVFrame, FrameDeleter>) = 0;

    virtual void CloseWriteing() = 0;

    virtual ~Maker() = default;

};

