/*ckwg +5
 * Copyright 2011 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "scoring_result.h"

#include <boost/make_shared.hpp>

namespace vistk
{

scoring_result
::scoring_result(count_t hit, count_t miss, count_t truth)
  : hit_count(hit)
  , miss_count(miss)
  , truth_count(truth)
{
}

scoring_result
::scoring_result(count_t hit, count_t miss, count_t truth, count_t possible)
  : hit_count(hit)
  , miss_count(miss)
  , truth_count(truth)
  , possible_count(possible)
{
}

scoring_result
::~scoring_result()
{
}

scoring_result::result_t
scoring_result
::percent_detection() const
{
  count_t const hit = hit_count;
  count_t const truth = truth_count;

  if (!truth)
  {
    return result_t(0);
  }

  return (result_t(hit) / result_t(truth));
}

scoring_result::result_t
scoring_result
::precision() const
{
  count_t const hit = hit_count;
  count_t const miss = miss_count;
  count_t const total = hit + miss;

  if (!total)
  {
    return result_t(0);
  }

  return (result_t(hit) / result_t(total));
}

scoring_result::result_t
scoring_result
::specificity() const
{
  if (!possible_count)
  {
    return result_t(0);
  }

  count_t const hit = hit_count;
  count_t const miss = miss_count;
  count_t const truth = truth_count;
  count_t const possible = *possible_count;
  count_t const truth_negative = possible - truth;
  count_t const true_negative = true_negative - miss;

  if (!truth_negative)
  {
    return result_t(0);
  }

  return (result_t(true_negative) / result_t(truth_negative));
}

scoring_result_t
operator + (scoring_result_t const& lhs, scoring_result_t const& rhs)
{
  if (lhs->possible_count && rhs->possible_count)
  {
    return boost::make_shared<scoring_result>(
      lhs->hit_count + rhs->hit_count,
      lhs->miss_count + rhs->miss_count,
      lhs->truth_count + rhs->truth_count,
      *lhs->possible_count + *rhs->possible_count);
  }

  return boost::make_shared<scoring_result>(
    lhs->hit_count + rhs->hit_count,
    lhs->miss_count + rhs->miss_count,
    lhs->truth_count + rhs->truth_count);
}

}
