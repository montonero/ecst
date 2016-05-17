// Copyright (c) 2015-2016 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#include <random>
#include <iostream>
#include <chrono>
#include <ecst.hpp>
#include "./settings_generator.hpp"

namespace example
{
    using vrm::core::uint;
    using vrm::core::sz_t;

    namespace component
    {
        struct position
        {
            int _x;
            int _y;
        };

        struct velocity
        {
            int _x;
            int _y;
        };

        struct acceleration
        {
            int _x;
            int _y;
        };

        struct counter
        {
            int _v;
        };

        struct countable
        {
        };
    }

    namespace c = example::component;

    namespace ct
    {
        namespace sct = ecst::signature::component;

        constexpr auto position = sct::tag<c::position>;
        constexpr auto velocity = sct::tag<c::velocity>;
        constexpr auto acceleration = sct::tag<c::acceleration>;
        constexpr auto counter = sct::tag<c::counter>;
        constexpr auto countable = sct::tag<c::countable>;
    }


    namespace actions
    {
        void accelerate(c::velocity& v, const c::acceleration& a)
        {
            v._x += a._x;
            v._y += a._y;
        }

        void move(c::position& p, const c::velocity& v)
        {
            p._x += v._x;
            p._y += v._y;
        }

        void count(c::counter& c)
        {
            c._v += 1;
        }
    }

    namespace system
    {
        struct acceleration
        {
            template <typename TData>
            void process(TData& data)
            {
                data.for_entities([&](auto eid)
                    {
                        auto& v = data.get(ct::velocity, eid);
                        const auto& a = data.get(ct::acceleration, eid);
                        actions::accelerate(v, a);
                    });
            }
        };

        struct velocity
        {
            template <typename TData>
            void process(TData& data)
            {
                data.for_entities([&](auto eid)
                    {
                        auto& p = data.get(ct::position, eid);
                        const auto& v = data.get(ct::velocity, eid);
                        actions::move(p, v);
                    });
            }
        };

        struct counter
        {
            template <typename TData>
            void process(TData& data)
            {
                data.for_entities([&](auto eid)
                    {
                        auto& c = data.get(ct::counter, eid);
                        actions::count(c);
                    });
            }
        };
    }

#define SYS_TAG(x)                                                  \
    namespace system                                                \
    {                                                               \
        struct x;                                                   \
    }                                                               \
    namespace st                                                    \
    {                                                               \
        constexpr auto x = ecst::signature::system::tag<system::x>; \
    }

    SYS_TAG(acceleration)
    SYS_TAG(velocity)
    SYS_TAG(counter)


    namespace ecst_setup
    {
        constexpr auto make_csl()
        {
            namespace c = example::component;
            namespace slc = ecst::signature_list::component;

            return slc::v<                                             // .
                c::position, c::velocity, c::acceleration, c::counter, // .
                c::countable                                           // .
                >;
        }

        constexpr auto entity_count = ecst::sz_v<1000>;

        constexpr auto make_ssl()
        {
            using ecst::sz_v;

            namespace c = example::component;
            namespace s = example::system;
            namespace ss = ecst::signature::system;
            namespace sls = ecst::signature_list::system;
            namespace ips = ecst::inner_parallelism::strategy;

            constexpr auto test_p = // .
                ips::split_every_n::v(sz_v<entity_count / 8>);

            constexpr auto ssig_acceleration =    // .
                ss::make<s::acceleration>(        // .
                    test_p,                       // .
                    ss::no_dependencies,          // .
                    ss::component_use(            // .
                        ss::mutate<c::velocity>,  // .
                        ss::read<c::acceleration> // .
                        ),                        // .
                    ss::output::none              // .
                    );

            constexpr auto ssig_velocity =           // .
                ss::make<s::velocity>(               // .
                    test_p,                          // .
                    ss::depends_on<s::acceleration>, // .
                    ss::component_use(               // .
                        ss::mutate<c::position>,     // .
                        ss::read<c::velocity>        // .
                        ),                           // .
                    ss::output::none                 // .
                    );

            constexpr auto ssig_counter =       // .
                ss::make<s::counter>(           // .
                    test_p,                     // .
                    ss::no_dependencies,        // .
                    ss::component_use(          // .
                        ss::mutate<c::counter>, // .
                        ss::read<c::countable>  // .
                        ),                      // .
                    ss::output::none            // .
                    );

            return sls::make(      // .
                ssig_counter,      // .
                ssig_acceleration, // .
                ssig_velocity      // .
                );
        }
    }

    namespace c = example::component;
    namespace s = example::system;

    auto test_impl = [](auto& ctx)
    {
        auto r_e0 = ctx.step([&ctx](auto& proxy)
            {
                auto e0 = proxy.create_entity();
                ECST_ASSERT(ctx.alive(e0));

                auto& e0_c_counter = proxy.add_component(ct::counter, e0);
                (void)e0_c_counter;
                proxy.add_component(ct::countable, e0);

                ECST_ASSERT(!ctx.is_in_system(st::counter, e0));

                return e0;
            });

        ECST_ASSERT(ctx.alive(r_e0));
        ECST_ASSERT(ctx.is_in_system(st::counter, r_e0));

        ctx.step([&ctx, r_e0](auto& proxy)
            {
                ECST_ASSERT(ctx.alive(r_e0));
                ECST_ASSERT(ctx.is_in_system(st::counter, r_e0));

                proxy.kill_entity(r_e0);

                ECST_ASSERT(ctx.alive(r_e0));
                ECST_ASSERT(ctx.is_in_system(st::counter, r_e0));
            });

        ECST_ASSERT(ctx.alive(r_e0) == false);
        ECST_ASSERT(!ctx.is_in_system(st::counter, r_e0));
    };
}

int main()
{
    using namespace example;
    using namespace example::ecst_setup;

    test::run_tests(test_impl, entity_count, make_csl(), make_ssl());
    return 0;
}
