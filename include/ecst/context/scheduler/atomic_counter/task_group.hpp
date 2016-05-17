// Copyright (c) 2015-2016 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: http://opensource.org/licenses/AFL-3.0
// http://vittorioromeo.info | vittorio.romeo@outlook.com

#pragma once

#include <ecst/config.hpp>
#include <ecst/aliases.hpp>
#include <ecst/utils.hpp>
#include "./task.hpp"

ECST_SCHEDULER_ATOMIC_COUNTER_NAMESPACE
{
    /// @brief Group of tasks.
    template <typename TTaskList>
    class task_group
    {
    public:
        static constexpr auto task_count() noexcept
        {
            return std::tuple_size<TTaskList>{};
        }

    private:
        TTaskList _tasks;

        template <typename TID>
        constexpr auto valid_id(TID id) const noexcept
        {
            return id >= 0 && id < task_count();
        }

    public:
        task_group() = default;
        ECST_DEFINE_DEFAULT_MOVE_ONLY_OPERATIONS(task_group);

        template <typename TID>
        auto& task_by_id(TID id) noexcept
        {
            ECST_ASSERT(valid_id(id));
            return std::get<id>(_tasks);
        }

        template <             // .
            typename TBlocker, // .
            typename TID,      // .
            typename TContext, // .
            typename TF        // .
            >
        void start_from_task_id(TBlocker& b, TID id, TContext& sp, TF&& f)
        {
            task_by_id(id).run(*this, b, id, sp, f);
        }
    };
}
ECST_SCHEDULER_ATOMIC_COUNTER_NAMESPACE_END
