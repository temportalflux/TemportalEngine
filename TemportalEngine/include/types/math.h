#pragma once

#define minUnless(x, max, unless) max != unless && x > max ? max : x
