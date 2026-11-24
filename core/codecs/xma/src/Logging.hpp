#pragma once

namespace AnyBox::Codecs {
void InitializeXmaLogging();

class XmaErrorScope {
public:
    XmaErrorScope() noexcept;
    ~XmaErrorScope();
    XmaErrorScope(const XmaErrorScope&) = delete;
    XmaErrorScope& operator=(const XmaErrorScope&) = delete;
    bool HasError() const noexcept;

private:
    bool failed = false;
    bool* previous;
};
}
