#include <threading.h>

void t_init()
{
        // TODO
        // For loop iterates through the contexts array and makes the state invalid except the 1st one which is the context for main
        for(int i = 0; i < NUM_CTX; i++){
                if(i != 0){
                        memset(&contexts[i], 0, sizeof(struct worker_context));
                        contexts[i].state = INVALID;
                }else{
                        memset(&contexts[i], 0, sizeof(struct worker_context));
                        contexts[i].state = VALID;
                }
        }
        current_context_idx = 0;
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        // TODO
        // Find a slot in the contexts array that is not set to valid, i.e., that is available
        volatile int idx = -1;
        volatile int i;
        for(i = 0; i < NUM_CTX; i++){
                if(contexts[i].state == INVALID){
                        idx = i;
                        break;
                }
        }

        if(idx == -1){
                return -1;  // No free slots available
        }

        // Sets the context and allocates stack space
        if(getcontext(&contexts[idx].context) == -1){
                return -1;
        }

        contexts[idx].context.uc_stack.ss_sp = (char*) malloc(STK_SZ);
        contexts[idx].context.uc_stack.ss_size = STK_SZ;
        contexts[idx].context.uc_stack.ss_flags = 0;
        contexts[idx].context.uc_link = NULL;

        // Changes the context to call the function from the arguments gives above
        makecontext(&contexts[idx].context, (ctx_ptr)foo, 2, arg1, arg2);

        // Changes the state of the context to VALID
        contexts[idx].state = VALID;

        return 0;
}

int32_t t_yield()
{
        // TODO
        // saves the index of the previous context
        int previous_context_idx = current_context_idx;
        if(getcontext(&contexts[current_context_idx].context) == -1){
                return -1;
        }

        // Search for the next context to be scheduled and also updates the current context index
        int valid_contexts = -1;
        for(int i = 1; i < NUM_CTX; i++){
                if(contexts[i].state == VALID){
                        current_context_idx = (uint8_t)i;
                        valid_contexts = i;
                        break;
                }
        }

        if(valid_contexts == -1){
                return -1;  // No contexts to schedule
        }

        // Swaps with the next valid context
        if(swapcontext(&contexts[previous_context_idx].context, &contexts[current_context_idx].context) == -1){
                return -1;
        }

        // Counts the number of valid contexts
        valid_contexts = 0;
        for(int i = 0; i < NUM_CTX; i++){
                if (contexts[i].state == VALID) {
                        valid_contexts++;
                }
        }

        return valid_contexts;
}

void t_finish()
{
        // TODO
        // Frees the heap using free function
        contexts[current_context_idx].state = DONE;
        free(contexts[current_context_idx].context.uc_stack.ss_sp);

        // Hands the control over to the next task
        t_yield();
}