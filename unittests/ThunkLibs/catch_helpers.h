#pragma once

#include <catch2/catch.hpp>

#include <string>
#include <tuple>

/**
 * Extracts the argument type from Catch matchers.
 *
 * Example use to get std::exception from an ExceptionMessageMatcher:
 *   using ArgType = decltype(GetMatchedTypeHelper(ExceptionMessageMatcher("test")))
 *
 * Calls to this function must be wrapped in decltype(), i.e. the result cannot not be evaluated.
 */
template<typename MatchedType>
MatchedType GetMatchedTypeHelper(const Catch::MatcherBase<MatchedType>&);

/**
 * Composite matcher that passes if all of the matchers given to it pass.
 *
 * Serves as a replacement for Catch's MatchAllOf, which does not take
 * ownership of its arguments and hence can run into lifetime problems
 * easily.
 */
template<typename MatchedType, typename... Matchers>
class matches_all : public Catch::MatcherBase<MatchedType> {
    // TODO: Can be an array of std::unique_ptr<MatcherBase> instead, since it has a virtual destructor
    std::tuple<Matchers...> matchers;

public:
    matches_all(Matchers&&... matchers_) :
        matchers { matchers_... } {
    }

    bool match(const MatchedType& arg) const override {
        return std::apply(
                    [&arg](auto&... matchers) { return (matchers.match(arg) && ...); },
                    matchers);
    }

    std::string describe() const override {
        std::string desc;
        std::apply(
            // For each matcher, include its description and add a combining ", and\n " suffix
            [&desc](auto&... matchers) { return ((desc += matchers.describe(), desc += ", and\n "), ...); },
            matchers);
        // Drop the last suffix
        desc.resize(desc.size() - 7);
        return desc;
    }
};

template<typename... Matchers>
matches_all(Matchers&&... matchers)
    -> matches_all<decltype(GetMatchedTypeHelper((matchers, ...))),
                Matchers...>;

/**
 * Inverting matcher. Succeeds if its argument fails to match.
 */
template<typename Matcher>
class not_matches : public Catch::MatcherBase<decltype(GetMatchedTypeHelper(std::declval<Matcher>()))> {
    Matcher matcher;

    using MatchedType = decltype(GetMatchedTypeHelper(std::declval<Matcher>()));

public:
    not_matches(Matcher&& matcher_) : matcher { matcher_ } {
    }

    bool match(const MatchedType& arg) const override {
        return !matcher.match(arg);
    }

    std::string describe() const override {
        return "NOT (" + matcher.describe() + ")";
    }
};
