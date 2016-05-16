
#include <nan.h>
#include <functional>
#include <iostream>
#include <assert.h>
#define BUFFERSIZE 65536

#include "MasterQueue.h"
#include "TestQueue.h"

using namespace Nan;
using namespace std;
using namespace v8;

#include "BatchRetrieveWorker.cpp"
#include "SpottingBatchUpdateWorker.cpp"
#include "TestBatchRetrieveWorker.cpp"
#include "SpottingTestBatchUpdateWorker.cpp"

MasterQueue* masterQueue;
TestQueue* testQueue;



NAN_METHOD(getNextBatch) {
    int width = To<int>(info[0]).FromJust();
    int num = To<int>(info[1]).FromJust();
    Callback *callback = new Callback(info[2].As<Function>());

    AsyncQueueWorker(new BatchRetrieveWorker(callback, width,num,masterQueue));
}

NAN_METHOD(spottingBatchDone) {//TODO
    //string batchId = To<string>(info[0]).FromJust();
    //string resultsId = To<string>(info[0]).FromJust();
    String::Utf8Value resultsIdNAN(info[0]);
    string resultsId = string(*resultsIdNAN);
    
    vector<string> ids;
    vector<int> labels;
    Handle<Value> val;
    if (info[1]->IsArray()) {
      Handle<Array> jsArray = Handle<Array>::Cast(info[1]);
      for (unsigned int i = 0; i < jsArray->Length(); i++) {
        val = jsArray->Get(i);
        ids.push_back(string(*String::Utf8Value(val)));
        //Nan::Set(arr, i, val);
      }
    }
    if (info[2]->IsArray()) {
      Handle<Array> jsArray = Handle<Array>::Cast(info[2]);
      for (unsigned int i = 0; i < jsArray->Length(); i++) {
        val = jsArray->Get(i);
        labels.push_back(val->Uint32Value());
        //Nan::Set(arr, i, val);
      }
    }
    
    Callback *callback = new Callback(info[2].As<Function>());

    AsyncQueueWorker(new SpottingBatchUpdateWorker(callback,masterQueue,resultsId,ids,labels));
}

NAN_METHOD(getNextTestBatch) {
    int width = To<int>(info[0]).FromJust();
    int num = To<int>(info[1]).FromJust();
    int userId = To<int>(info[2]).FromJust();
    Callback *callback = new Callback(info[3].As<Function>());

    AsyncQueueWorker(new TestBatchRetrieveWorker(callback, width,num,userId,testQueue));
}

NAN_METHOD(spottingTestBatchDone) {
    String::Utf8Value resultsIdNAN(info[0]);
    string resultsId = string(*resultsIdNAN);
    
    vector<string> ids;
    vector<int> labels;
    Handle<Value> val;
    if (info[1]->IsArray()) {
      Handle<Array> jsArray = Handle<Array>::Cast(info[1]);
      for (unsigned int i = 0; i < jsArray->Length(); i++) {
        val = jsArray->Get(i);
        ids.push_back(string(*String::Utf8Value(val)));
      }
    }
    if (info[2]->IsArray()) {
      Handle<Array> jsArray = Handle<Array>::Cast(info[2]);
      for (unsigned int i = 0; i < jsArray->Length(); i++) {
        val = jsArray->Get(i);
        labels.push_back(val->Uint32Value());
      }
    }
    int userId = To<int>(info[2]).FromJust();
    Callback *callback = new Callback(info[3].As<Function>());

    AsyncQueueWorker(new SpottingTestBatchUpdateWorker(callback,testQueue,resultsId,ids,labels,userId));
}

NAN_METHOD(resetTestUsers) {
    
    Callback *callback = new Callback(info[0].As<Function>());
    AsyncQueueWorker(new ResetTestUsersWorker(callback,testQueue));
}

NAN_MODULE_INIT(Init) {
    
    masterQueue = new MasterQueue();
    
    
    Nan::Set(target, New<String>("getNextBatch").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(getNextBatch)).ToLocalChecked());
    
    Nan::Set(target, New<String>("spottingBatchDone").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(spottingBatchDone)).ToLocalChecked());
    
    testQueue = new TestQueue();
    
    Nan::Set(target, New<String>("getNextTestBatch").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(getNextBatch)).ToLocalChecked());
    
    Nan::Set(target, New<String>("spottingTestBatchDone").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(spottingBatchDone)).ToLocalChecked());
    
    Nan::Set(target, New<String>("resetTestUsers").ToLocalChecked(),
        GetFunction(New<FunctionTemplate>(resetTestUsers)).ToLocalChecked());
}

NODE_MODULE(SpottingAddon, Init)

