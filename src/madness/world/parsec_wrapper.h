#ifndef PARSEC_WRAPPER_H__INCLUDED
#define PARSEC_WRAPPER_H__INCLUDED

#include "parsec.h"
#include "parsec/class/parsec_object.h"

namespace Parsec {
    class Handle {
    protected:
        parsec_handle_t  *parsec_handle_;
        parsec_context_t *parsec_context_;
        bool             handle_scheduled;

    public:
        Handle(parsec_context_t *context) :
            parsec_handle_(NULL),
            parsec_context_(context),
            handle_scheduled(false)
        {
        }

        ~Handle() {
            if( NULL != parsec_handle_ ) {
                /** For a reason, probably because it was not
                 *  scheduled, the handle was not destructed by
                 *  the specialized class. Try to memory collect
                 *  using generic parsec destructor */
                OBJ_RELEASE(parsec_handle_);
            }
            parsec_handle_ = NULL;
            parsec_context_ = NULL;
        }
    };
    
    class Parsec {
    private:
        parsec_context_t *parsec_context_;
        int              nb_cores_;
        void initialize(std::vector<std::string> &v) {
            int argc = v.size();
            char **argv = new char*[argc+2];
            char **p = argv;
            *p++ = strdup("parsec_wrapper");
            for(auto s = v.begin(); s != v.end(); ++s) {
                *p++ = strdup(s->c_str());
            }
            *p = NULL;
            parsec_context_ = parsec_init(nb_cores_, &argc, &argv);
            for(int a = 0; a < argc; a++)
                free(argv[a]);
            delete[] argv;
        }

    public:
        Parsec(void) {
            std::vector<std::string> empty_args;
            nb_cores_ = -1;
            initialize(empty_args);
        }
        Parsec(int nb_cores) {
            std::vector<std::string> empty_args;
            nb_cores_ = nb_cores;
            initialize(empty_args);
        }
        Parsec(std::vector<std::string> &v) {
            nb_cores_ = -1;
            initialize(v);
        }
        Parsec(int nb_cores, std::vector<std::string> &v) {
            nb_cores_ = nb_cores;
            initialize(v);
        }

        parsec_context_t *context(void) {
            return parsec_context_;
        }

        void Finalize(void) {
            parsec_fini(&parsec_context_);
        }
        
        ~Parsec() {
        }
    }; // class Parsec
    
} //namespace Parsec

#endif /* PARSEC_WRAPPER_H__INCLUDED */
