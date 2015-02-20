#include <stdio.h>
#include <stdlib.h>
#include "node_pointer.h"

#include <nn.h>
#include <pubsub.h>
#include <pipeline.h>
#include <bus.h>
#include <pair.h>
#include <reqrep.h>
#include <survey.h>
#include <inproc.h>
#include <ipc.h>
#include <tcp.h>

using v8::Array;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::Local;
using v8::Persistent;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

NAN_METHOD(Socket) {
    NanScope();

    int domain = args[0]->Uint32Value();
    int protocol = args[1]->Uint32Value();

    // Invoke nanomsg function.
    int ret = nn_socket(domain, protocol);

    if( (ret >= 0) && (protocol == NN_SUB)) {
        if (nn_setsockopt(ret, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) != 0) {
            return NanThrowError("Could not set subscribe option.");
        }
    }

    NanReturnValue(NanNew<Number>(ret));
}


NAN_METHOD(Close) {
    NanScope();

    int s = args[0]->Uint32Value();

    // Invoke nanomsg function.
    int ret = nn_close(s);

    NanReturnValue(NanNew<Number>(ret));
}


NAN_METHOD(Setsockopt) {
    NanScope();

    int s = args[0]->Uint32Value();
    int level = args[1]->Uint32Value();
    int option = args[2]->Uint32Value();
    int ret;

    switch(option) {
        /* string setters */
        case NN_SOCKET_NAME:
            {
                String::Utf8Value str(args[3]);

                // Invoke nanomsg function.
                ret = nn_setsockopt(s, level, option, *str, str.length());
            }
            break;

        /* int setters */
        default:
            {
                int optval = args[3]->Uint32Value();

                // Invoke nanomsg function.
                ret = nn_setsockopt(s, level, option, &optval, sizeof(optval));
            }
            break;
    }

    NanReturnValue(NanNew<Number>(ret));
}


// returns an array n where:
// n[0] is the return code (0 good, negative bad)
// n[1] is an int or string representing the option's value
NAN_METHOD(Getsockopt) {
    NanScope();

    int s = args[0]->Uint32Value();
    int level = args[1]->Uint32Value();
    int option = args[2]->Uint32Value();
    //int optval = args[3]->Uint32Value();
    int optval[64];

    // Invoke nanomsg function.
    size_t optsize = sizeof(optval);
    int ret = nn_getsockopt(s, level, option, optval, &optsize);

    Local<Array> obj = NanNew<Array>(2);
    obj->Set(0, NanNew<Number>(ret));

    if(ret == 0) {
        switch(option) {
            /* string return values */
            case NN_SOCKET_NAME:
                obj->Set(1, NanNew<String>((char *)optval));
                break;

            /* int return values */
            default:
                obj->Set(1, NanNew<Number>(optval[0]));
                break;
        }
    }

    // otherwise pass the error back
    NanReturnValue(obj);
}

struct BindState {
    BindState(int sock_, Handle<Function> cb_, Handle<String> addr_) : addr(addr_) {
        sock = sock_;
        NanAssignPersistent(cb, cb_);
        error = 0;
        endpoint = 0;
    }

    ~BindState() {
        NanDisposePersistent(cb);
    }

    int sock;
    Persistent<Function> cb;
    String::Utf8Value addr;
    int error;
    int endpoint;
};

void UV_BindAsync(uv_work_t *req) {
    BindState *state = static_cast<BindState *>(req->data);
    int ep = nn_bind(state->sock, *state->addr);
    if (ep < 0) {
        state->error = nn_errno();
    }
}

void UV_BindAsyncAfter(uv_work_t *req) {
    BindState *state = static_cast<BindState *>(req->data);
    NanScope();
    Local<Value> argv[2];
    if (state->error) {
        argv[0] = NanUndefined();
        argv[1] = NanError(nn_strerror(state->error));
    } else {
        argv[0] = NanNew<Number>(state->endpoint);
        argv[1] = NanUndefined();
    }
    Local<Function> cb = NanNew(state->cb);
    NanMakeCallback(NanGetCurrentContext()->Global(), cb, 2, argv);
    delete state;
    delete req;
}

NAN_METHOD(Bind) {
    NanScope();
    if (!args[0]->IsNumber()) {
        return NanThrowTypeError("Socket must be a number!");
    }
    if (!args[1]->IsString()) {
        return NanThrowTypeError("Address must be a string!");
    }
    if (args.Length() > 2 && !args[2]->IsFunction()) {
        return NanThrowTypeError("Callback must be a function!");
    }

    int s = args[0]->Uint32Value();
    Local<String> addr = args[1].As<String>();
    Local<Function> cb = Local<Function>::Cast(args[2]);

    BindState *state = new BindState(s, cb, addr);
    uv_work_t *req = new uv_work_t;
    req->data = state;
    uv_queue_work(uv_default_loop(), req, UV_BindAsync, (uv_after_work_cb)UV_BindAsyncAfter);

    NanReturnUndefined();
}

NAN_METHOD(BindSync) {
    NanScope();
    if (!args[0]->IsNumber()) {
        return NanThrowTypeError("Socket must be a number!");
    }
    if (!args[1]->IsString()) {
        return NanThrowTypeError("Address must be a string!");
    }

    int s = args[0]->Uint32Value();
    String::Utf8Value addr(args[1]);

    // Invoke nanomsg function.
    int ret = nn_bind(s, *addr);

    NanReturnValue(NanNew<Number>(ret));
}

NAN_METHOD(Connect) {
    NanScope();
    if (!args[0]->IsNumber()) {
        return NanThrowTypeError("Socket must be a number!");
    }
    if (!args[1]->IsString()) {
        return NanThrowTypeError("Address must be a string!");
    }
    if (args.Length() > 2 && !args[2]->IsFunction()) {
        return NanThrowTypeError("Callback must be a function!");
    }

    int s = args[0]->Uint32Value();
    String::Utf8Value addr(args[1]);

    // Invoke nanomsg function.
    int ret = nn_connect(s, *addr);

    NanReturnValue(NanNew<Number>(ret));
}

NAN_METHOD(ConnectSync) {
    NanScope();
    if (!args[0]->IsNumber()) {
        return NanThrowTypeError("Socket must be a number!");
    }
    if (!args[1]->IsString()) {
        return NanThrowTypeError("Address must be a string!");
    }

    int s = args[0]->Uint32Value();
    String::Utf8Value addr(args[1]);

    // Invoke nanomsg function.
    int ret = nn_connect(s, *addr);

    NanReturnValue(NanNew<Number>(ret));
}

NAN_METHOD(Shutdown) {
    NanScope();

    int s = args[0]->Uint32Value();
    int how = args[1]->Uint32Value();

    // Invoke nanomsg function.
    int ret = nn_shutdown(s, how);

    NanReturnValue(NanNew<Number>(ret));
}


NAN_METHOD(Send) {
    NanScope();

    int s = args[0]->Uint32Value();

    if(!node::Buffer::HasInstance(args[1])) {
        return NanThrowError("second Argument must be a Buffer.");
    }

    Local<Object> obj = args[1]->ToObject();
    char* odata = node::Buffer::Data(obj);
    size_t odata_len = node::Buffer::Length(obj);
    int flags = args[2]->Uint32Value();

    // Invoke nanomsg function.
    int ret = nn_send(s, odata, odata_len, flags);

    NanReturnValue(NanNew<Number>(ret));
}


NAN_METHOD(Recv) {
    NanScope();

    int s = args[0]->Uint32Value();
    int flags = args[1]->Uint32Value();

    // Invoke nanomsg function.
    void *retbuf = NULL;
    int ret = nn_recv(s, &retbuf, NN_MSG, flags);

    // TODO multiple return args
    if(ret > -1) {
        NanReturnValue(NanNewBufferHandle((char*) retbuf, ret));
    } else {
        NanReturnValue(NanNew<Number>(ret));
    }
}

NAN_METHOD(SymbolInfo) {
    NanScope();

    int s = args[0]->Uint32Value();
    struct nn_symbol_properties prop;
    int ret = nn_symbol_info(s, &prop, sizeof(prop));

    if(ret > 0) {
        Local<Object> obj = NanNew<Object>();
        obj->Set(NanNew("value"), NanNew<Number>(prop.value));
        obj->Set(NanNew("ns"), NanNew<Number>(prop.ns));
        obj->Set(NanNew("type"), NanNew<Number>(prop.type));
        obj->Set(NanNew("unit"), NanNew<Number>(prop.unit));
        obj->Set(NanNew("name"), NanNew<String>(prop.name));
        NanReturnValue(obj);
    }
    else if(ret == 0) {
        // symbol index out of range
        NanReturnUndefined();
    } else {
        NanThrowError(nn_strerror(nn_errno()));
        NanReturnUndefined();
    }
}

NAN_METHOD(Symbol) {
    NanScope();

    int s = args[0]->Uint32Value();
    int val;
    const char *ret = nn_symbol(s, &val);

    if(ret) {
        Local<Object> obj = NanNew<Object>();
        obj->Set(NanNew("value"), NanNew<Number>(val));
        obj->Set(NanNew("name"), NanNew<String>(ret));
        NanReturnValue(obj);
    } else {
        // symbol index out of range
        // this behaviour seems inconsistent with SymbolInfo() above
        // but we are faithfully following the libnanomsg API, warta and all
        NanThrowError(nn_strerror(nn_errno())); // EINVAL
        NanReturnUndefined();
    }
}

NAN_METHOD(Term) {
    NanScope();
    nn_term();
    NanReturnUndefined();
}

// Pass in two sockets, or (socket, -1) or (-1, socket) for loopback
NAN_METHOD(Device) {
    NanScope();
    int s1 = args[0]->Uint32Value();
    int s2 = args[1]->Uint32Value();

    // nn_device only returns when it encounters an error
    nn_device(s1, s2);
    NanThrowError(nn_strerror(nn_errno()));
    NanReturnUndefined();
}

NAN_METHOD(Errno) {
    NanScope();

    // Invoke nanomsg function.
    int ret = nn_errno();

    NanReturnValue(NanNew<Number>(ret));
}


NAN_METHOD(Strerr) {
    NanScope();

    int errnum = args[0]->Uint32Value();

    // Invoke nanomsg function.
    const char* err = nn_strerror(errnum);

    NanReturnValue(NanNew<String>(err));
}


typedef struct nanomsg_socket_s {
    uv_poll_t poll_handle;
    uv_os_sock_t sockfd;
    NanCallback* callback;
} nanomsg_socket_t;

void NanomsgReadable (uv_poll_t *req, int status, int events) {
    NanScope();
    nanomsg_socket_t *context;
    context = reinterpret_cast<nanomsg_socket_t*>(req);

    if (events & UV_READABLE) {
        Local<Value> argv[] = {
            NanNew<Number>(events)
        };
        context->callback->Call(1, argv);
    }
}

NAN_METHOD(PollSendSocket) {
    NanScope();

    int s = args[0]->Uint32Value();
    NanCallback* callback = new NanCallback(args[1].As<Function>());

    nanomsg_socket_t *context;
    size_t siz = sizeof(uv_os_sock_t);

    context = reinterpret_cast<nanomsg_socket_t*>(calloc(1, sizeof *context));
    context->poll_handle.data = context;
    context->callback = callback;
    nn_getsockopt(s, NN_SOL_SOCKET, NN_SNDFD, &context->sockfd, &siz);

    if (context->sockfd != 0) {
        uv_poll_init_socket(uv_default_loop(), &context->poll_handle, context->sockfd);
        uv_poll_start(&context->poll_handle, UV_READABLE, NanomsgReadable);
        NanReturnValue(WrapPointer(context, 8));
    } else {
        NanReturnUndefined();
    }
}

NAN_METHOD(PollReceiveSocket) {
    NanScope();

    int s = args[0]->Uint32Value();
    NanCallback* callback = new NanCallback(args[1].As<Function>());

    nanomsg_socket_t *context;
    size_t siz = sizeof(uv_os_sock_t);

    context = reinterpret_cast<nanomsg_socket_t*>(calloc(1, sizeof *context));
    context->poll_handle.data = context;
    context->callback = callback;
    nn_getsockopt(s, NN_SOL_SOCKET, NN_RCVFD, &context->sockfd, &siz);

    if (context->sockfd != 0) {
        uv_poll_init_socket(uv_default_loop(), &context->poll_handle, context->sockfd);
        uv_poll_start(&context->poll_handle, UV_READABLE, NanomsgReadable);
        NanReturnValue(WrapPointer(context, 8));
    } else {
        NanReturnUndefined();
    }
}

NAN_METHOD(PollStop) {
    NanScope();

    nanomsg_socket_t *context = UnwrapPointer<nanomsg_socket_t*>(args[0]);
    int r = uv_poll_stop(&context->poll_handle);
    NanReturnValue(NanNew<Number>(r));
}

class NanomsgDeviceWorker : public NanAsyncWorker {
    public:
        NanomsgDeviceWorker(NanCallback *callback, int s1, int s2)
            : NanAsyncWorker(callback), s1(s1), s2(s2) {}
        ~NanomsgDeviceWorker() {}

        // Executed inside the worker-thread.
        // It is not safe to access V8, or V8 data structures
        // here, so everything we need for input and output
        // should go on `this`.
        void Execute() {
            // nn_errno() only returns on error
            nn_device(s1, s2);
            err = nn_errno();
        }

        // Executed when the async work is complete
        // this function will be run inside the main event loop
        // so it is safe to use V8 again
        void HandleOKCallback() {
            NanScope();

            Local<Value> argv[] = {
                NanNew<Number>(err)
            };

            callback->Call(1, argv);
        };

    private:
        int s1;
        int s2;
        int err;
};

// Asynchronous access to the `nn_device()` function
NAN_METHOD(DeviceWorker) {
    NanScope();

    int s1 = args[0]->Uint32Value();
    int s2 = args[1]->Uint32Value();
    NanCallback *callback = new NanCallback(args[2].As<Function>());

    NanAsyncQueueWorker(new NanomsgDeviceWorker(callback, s1, s2));
    NanReturnUndefined();
}

#define EXPORT_METHOD(C, S) C->Set(NanNew(# S), NanNew<FunctionTemplate>(S)->GetFunction());

void InitAll(Handle<Object> exports) {
    NanScope();

    // Export functions.
    EXPORT_METHOD(exports, Socket);
    EXPORT_METHOD(exports, Close);
    EXPORT_METHOD(exports, Setsockopt);
    EXPORT_METHOD(exports, Getsockopt);
    EXPORT_METHOD(exports, Bind);
    EXPORT_METHOD(exports, BindSync);
    EXPORT_METHOD(exports, Connect);
    EXPORT_METHOD(exports, ConnectSync);
    EXPORT_METHOD(exports, Shutdown);
    EXPORT_METHOD(exports, Send);
    EXPORT_METHOD(exports, Recv);
    EXPORT_METHOD(exports, Errno);
    EXPORT_METHOD(exports, Strerr);
    EXPORT_METHOD(exports, PollSendSocket);
    EXPORT_METHOD(exports, PollReceiveSocket);
    EXPORT_METHOD(exports, PollStop);
    EXPORT_METHOD(exports, DeviceWorker);
    EXPORT_METHOD(exports, SymbolInfo);
    EXPORT_METHOD(exports, Symbol);
    EXPORT_METHOD(exports, Term);

    // Export symbols.
    int index = 0;
    int value;
    while (true) {
        const char *symbol_name = nn_symbol(index, &value);
        if (symbol_name == NULL) {
            break;
        }
        exports->Set(NanNew(symbol_name), NanNew<Number>(value));
        index += 1;
    }
}

NODE_MODULE(node_nanomsg, InitAll)
