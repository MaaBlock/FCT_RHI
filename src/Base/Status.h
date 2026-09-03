#pragma once

#ifndef FCT_STATUS_H
#define FCT_STATUS_H

#include <absl/status/status.h>
#include <absl/status/statusor.h>
#include <absl/strings/str_cat.h>

namespace FCT {

    using Status = absl::Status;

    template<typename T>
    using StatusOr = absl::StatusOr<T>;

    using absl::OkStatus;
    using absl::InvalidArgumentError;
    using absl::FailedPreconditionError;
    using absl::NotFoundError;
    using absl::AlreadyExistsError;
    using absl::InternalError;
    using absl::UnavailableError;
    using absl::UnimplementedError;
    using absl::OutOfRangeError;
    using absl::UnknownError;
    using absl::StrCat;

    #define FCT_CHECK_STATUS(expr) \
        do { \
            auto&& _status_eval = (expr); \
            if (!_status_eval.ok()) { \
                return _status_eval; \
            } \
        } while (0)

} // namespace FCT

#endif // FCT_STATUS_H
