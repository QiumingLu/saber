// Copyright (c) 2017 Mirants Lu. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SABER_PUBLIC_VERSION_H_
#define SABER_PUBLIC_VERSION_H_

// Saber uses semantic versioning, see http://semver.org/.

#define SABER_MAJOR_VERSION 1
#define SABER_MINOR_VERSION 0
#define SABER_PATCH_VERSION 3

// SABER_VERSION_SUFFIX is non-empty for pre-releases (e.g. "-alpha",
// "-alpha.1",
// "-beta", "-rc", "-rc.1")
#define SABER_VERSION_SUFFIX ""

#define SABER_STR_HELPER(x) #x
#define SABER_STR(x) SABER_STR_HELPER(x)

// e.g. "1.0.0" or "1.0.0-alpha".
#define SABER_VERSION_STRING                                  \
  (SABER_STR(SABER_MAJOR_VERSION) "." SABER_STR(              \
      SABER_MINOR_VERSION) "." SABER_STR(SABER_PATCH_VERSION) \
       SABER_VERSION_SUFFIX)

#endif  // SABER_PUBLIC_VERSION_H_
