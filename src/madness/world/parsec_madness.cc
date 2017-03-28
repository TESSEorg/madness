#include <madness/madness_config.h>

#ifdef HAVE_PARSEC
#include "parsec_madness.h"
#include <iostream>
#include "thread.h"
#include <parsec/parsec_internal.h>

// Here we initialize with the right child class
namespace madness {
    parsec_hook_return_t complete_madness_task_execution (parsec_execution_unit_t *eu, 
                                                         parsec_execution_context_t *exec_context)
    {
        (void)eu;
        parsec_atomic_add_32b(&exec_context->parsec_handle->nb_tasks, -1);
        return PARSEC_HOOK_RETURN_DONE;
    }

    parsec_hook_return_t release_madness_task (parsec_execution_unit_t *eu, 
                                              parsec_execution_context_t *exec_context)
    {
        PoolTaskInterface *c = ((PoolTaskInterface **)exec_context->locals)[0];
        delete(c);
        return PARSEC_HOOK_RETURN_DONE;
    }

    const parsec_function_t* PoolTaskInterface::func = &madness_function;

    parsec_hook_return_t pointer_call(parsec_execution_unit_t *eu, 
                                     parsec_execution_context_t *exec_context)
    {
        PoolTaskInterface *c = ((PoolTaskInterface **)exec_context->locals)[0];
        c->run(TaskThreadEnv(1, 0, 0));
        return PARSEC_HOOK_RETURN_DONE;
    }


  extern "C"{

#include <stdio.h>

        static uint64_t RUN_hash(const parsec_handle_t *handle, const assignment_t *a)
        {
            (void)a; (void)handle;
            return 0;
        }

        static parsec_hook_return_t empty_hook (parsec_execution_unit_t *eu, 
                                               parsec_execution_context_t *context)
        {
            return PARSEC_HOOK_RETURN_DONE;
        }

        static const __parsec_chore_t __RUN_chores[] = {
            {.type = PARSEC_DEV_CPU,
             .evaluate = NULL,
             .hook = pointer_call},
            {.type = PARSEC_DEV_NONE,
             .evaluate = NULL,
             .hook = (parsec_hook_t *) NULL},
        };


        const parsec_function_t madness_function = {
            .name = "RUN",
            .flags = PARSEC_HAS_IN_IN_DEPENDENCIES | PARSEC_USE_DEPS_MASK,
            .function_id = 0,
            .nb_flows = 0,
            .nb_parameters = 0,
            .nb_locals = 0,
            .dependencies_goal = 0,
            .params = {NULL,NULL,NULL, NULL},
            .locals = {NULL,NULL,NULL, NULL},
            .in = {NULL, NULL, NULL, NULL},
            .out = {NULL, NULL, NULL, NULL},
            .priority = NULL,
            .initial_data = NULL,
            .final_data = NULL,
            .data_affinity = NULL,
            .key = (parsec_functionkey_fn_t *) RUN_hash,
#if defined(PARSEC_SIM)
            .sim_cost_fct = (parsec_sim_cost_fct_t*) NULL,
#endif
            .get_datatype = (parsec_datatype_lookup_t *) NULL,
            .prepare_input = empty_hook,
            .incarnations = __RUN_chores,
            .prepare_output = (parsec_hook_t *) NULL,
            .find_deps = (parsec_find_dependency_fn_t*)NULL,
            .iterate_successors = (parsec_traverse_function_t *) NULL,
            .iterate_predecessors = (parsec_traverse_function_t *) NULL,
            .release_deps = (parsec_release_deps_t *) NULL,
            .complete_execution = complete_madness_task_execution,
            .new_task = (parsec_new_task_function_t*) NULL,
            .release_task = release_madness_task, //object delete,
            .fini = (parsec_hook_t *) NULL,
        };

        const parsec_function_t* madness_function_array[]= {&(madness::madness_function), NULL};

        parsec_handle_t madness_handle = {
            .super = { 0x0, },
            .handle_id = 0,
            .nb_tasks = 0,
            .nb_functions = 1,
            .devices_mask = PARSEC_DEVICES_ALL,
            .initial_number_tasks = 0,
            .priority = 0,
            .nb_pending_actions = 1,
            .context = NULL,
            .startup_hook = NULL,
            .functions_array = madness_function_array
        };
    }
}
#endif
