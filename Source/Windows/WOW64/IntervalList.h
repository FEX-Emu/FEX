// SPDX-License-Identifier: MIT
#pragma once

#include <utility>
#include <algorithm>

#include <FEXCore/fextl/vector.h>

template<typename SizeType>
class IntervalList {
public:
  using DifferenceType = decltype(std::declval<SizeType>() - std::declval<SizeType>());

  struct Interval {
    SizeType Offset;
    SizeType End;

    Interval() = default;

    Interval(SizeType Offset, SizeType End) : Offset{Offset}, End{End} {}
  };

private:
  fextl::vector<Interval> Intervals; ///< list of intervals sorted by their end offset

public:
  struct QueryResult {
    bool Enclosed; ///< If the given offset was enclosed by an interval
    DifferenceType Size; ///< Size of the interval starting from the query offset, or distance to the next interval if
                         /// `Enclosed` is false (if there is no next interval, size is 0)
  };

  void Clear() {
    Intervals.clear();
  }

  void Insert(Interval Entry) {
    if (Entry.Offset == Entry.End) {
      return;
    }

    auto [FirstIt, EndIt] = std::equal_range(Intervals.begin(), Intervals.end(), Entry, [](const auto &LHS, const auto &RHS) {
      return LHS.End <= RHS.Offset;
    });

    if (FirstIt == EndIt) {
      // No overlaps
      Intervals.insert(FirstIt, Entry);
      return;
    }

    auto LastIt = std::prev(EndIt);
    // FirstIt/LastIt are the lowest/highest offset intervals respectively that overlap with the new interval

    const SizeType Offset = std::min(Entry.Offset, FirstIt->Offset);
    const SizeType End = std::max(LastIt->End, Entry.End);

    // Erase all overlapping entries but the first
    const auto EraseStartIt = std::next(FirstIt);
    const auto EraseEndIt = std::next(LastIt);
    LastIt = Intervals.erase(EraseStartIt, EraseEndIt);
    FirstIt = std::prev(LastIt);

    FirstIt->Offset = Offset;
    FirstIt->End = End;
  }

  void Remove(Interval Entry) {
    if (Entry.Offset == Entry.End) {
      return;
    }

    auto [FirstIt, EndIt] = std::equal_range(Intervals.begin(), Intervals.end(), Entry, [](const auto &LHS, const auto &RHS) {
      return LHS.End <= RHS.Offset;
    });

    if (FirstIt == EndIt) {
      // No intersecting intervals present, nothing more to do
      return;
    }

    if (FirstIt->Offset < Entry.Offset && FirstIt->End > Entry.End) {
      // The interval to be removed is fully enclosed by an existing interval
      
      // Break the single interval into two smaller intervals on either side on the interval being removed
      const auto FirstPredecessorIt = Intervals.insert(FirstIt, *FirstIt);
      FirstIt = std::next(FirstPredecessorIt);
      FirstPredecessorIt->End = Entry.Offset;
      FirstIt->Offset = Entry.End;
      return;
    }

    auto LastIt = std::prev(EndIt);
    // FirstIt/LastIt are the lowest/highest offset intervals respectively that overlap with the new interval

    if (FirstIt->Offset < Entry.Offset) {
      // The first overlap straddles the start of the interval to be removed
      FirstIt->End = Entry.Offset;
      if (FirstIt == LastIt) {
        // No more overlaps left, nothing more to do
        return;
      } else {
        FirstIt++;
      }
    }

    if (LastIt->End > Entry.End) {
      // The last overlap straddles the end of the interval to be removed
      LastIt->Offset = Entry.End;
      if (LastIt == FirstIt) {
        // No more overlaps left, nothing more to do
        return;
      } else {
        LastIt--;
      }
    }

    // Now none of the overlaps straddle the edges of the interval to be removed they can all be erased
    const auto EraseStartIt = FirstIt;
    const auto EraseEndIt = std::next(LastIt);
    Intervals.erase(EraseStartIt, EraseEndIt);
  }

  QueryResult Query(SizeType Offset) {
    const auto It = std::upper_bound(Intervals.begin(), Intervals.end(), Offset, [](const auto &LHS, const auto &RHS) {
      return LHS < RHS.End;
    }); // Lowest offset interval that (maybe) overlaps with the query offset

    if (It == Intervals.end()) { // No overlaps past offset
      return {false, {}};
    } else if (It->Offset > Offset) { // No overlap, return the distance to the next possible overlap
      return {false, It->Offset - Offset};
    } else { // Overlap, return the distance to the end of the overlap
      return {true, It->End - Offset};
    }
  }

  bool Intersect(Interval Entry) {
    const auto It = std::upper_bound(Intervals.begin(), Intervals.end(), Entry, [](const auto &LHS, const auto &RHS) {
      return LHS.Offset < RHS.End;
    }); // Lowest offset interval that (maybe) overlaps with the query offset

    return It != Intervals.end() && It->Offset < Entry.End;
  }
};
