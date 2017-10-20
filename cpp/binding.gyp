
{
  "targets": [
    {
      "target_name": "spottingaddon",
      "sources": [  "SpottingAddon.cpp", 
                    "BatchRetrieveWorker.cpp", 
                    "SpottingBatchUpdateWorker.cpp", 
                    "MasterQueue.h", "MasterQueue.cpp", 
                    "Batcher.h",
                    "ClusterBatcher.h", "ClusterBatcher.cpp",
                    "SpottingResults.h", "SpottingResults.cpp", 
                    "BatchWraper.h", 
                    "BatchWraperSpottings.h", "BatchWraperSpottings.cpp", 
                    "BatchWraperTranscription.h", "BatchWraperTranscription.cpp", 
                    "TranscribeBatchQueue.h", "TranscribeBatchQueue.cpp", 
                    "Knowledge.h","Knowledge.cpp", 
                    "Lexicon.h", "Lexicon.cpp", 
                    "MiscWorker.cpp", 
                    "CATTSS.h", "CATTSS.cpp", 
                    "Global.h", "Global.cpp", 
                    "Lexicon.h", "Lexicon.cpp", 
                    "NewExemplarsBatchQueue.h", "NewExemplarsBatchQueue.cpp", 
                    "BatchWraperNewExemplars.h", "BatchWraperNewExemplars.cpp", 
                    "NewExemplarsBatchUpdateWorker.cpp", 
                    "maxflow/graph.h", "maxflow/graph.cpp", "maxflow/block.h", "maxflow/instances.inc", "maxflow/maxflow.cpp", 
                    "batches.h", "batches.cpp", 
                    "spotting.h", 
                    "CorpusRef.h", "PageRef.h",
                    "WordBackPointer.h",
                    "SpottingQuery.h"
                    "SpottingQueue.h", "SpottingQueue.cpp",
                    "Spotter.h",
                    "NetSpotter.h", "NetSpotter.cpp",
                    "CTCWrapper.h", "CTCWrapper.cpp",
                    "Web.h", "Web.cpp",

                    "SpecialInstances.h",
                    "TestingInstances.h", "TestingInstances.cpp",
                    "TrainingInstances.h", "TrainingInstances.cpp",
                    "TrainingBatchWraperSpottings.h", "TrainingBatchWraperSpottings.cpp",
                    "TrainingBatchWraperTranscription.h", "TrainingBatchWraperTranscription.cpp",
                    "SpecialBatchRetrieveWorker.cpp"
                ],
      "old_sources": [ "AlmazanSpotter.h", "AlmazanSpotter.cpp", "AlmazanDataset.h", "AlmazanDataset.cpp" ],
      "cflags": ["-Wall", "-std=c++11", "-fexceptions", "-DOPENCV2", "-DCPU_ONLY", "-DCTC"], 
      'old_flags' : ["-DTEST_MODE", "-DGRAPH_SPOTTING_RESULTS"],
      'cflags!': [ '-fno-exceptions' ,'-fno-rtti'],
      'cflags_cc!': [ '-fno-exceptions', '-fno-rtti' ],
      "include_dirs" : ["<!(node -e \"require('nan')\")", 
            "/home/brian/Projects/brian_caffe/scripts/cnnspp_spotter", 
            "/home/brian/robert_stuff/documentproject/src",
            "/home/brian/Projects/brian_caffe/include"
          ],
      "libraries": [
            "-lopencv_highgui", "-lb64", "-pthread", "-lopencv_imgproc", "-fopenmp", 
            "-L/home/brian/robert_stuff/documentproject/lib", "-Wl,-rpath=/home/brian/robert_stuff/documentproject/lib", "-ldtwarp",
            "-L/home/brian/Projects/brian_caffe/scripts/cnnspp_spotter", "-Wl,-rpath=/home/brian/Projects/brian_caffe/scripts/cnnspp_spotter", "-lcnnspp_spotter",
            "-L/home/brian/Projects/brian_caffe/build/lib", "-lcaffe", "-lboost_system"
          ],
      "old_include_dirs" : ["<!(node -e \"require('nan')\")", "/home/brian/intel_index/EmbAttSpotter"],
      "old_libraries": [
            "-lopencv_highgui", "-lb64", "-pthread", "-lopencv_imgproc", "-fopenmp", 
            "-L/home/brian/intel_index/EmbAttSpotter", "-lembattspotter",
            "-L/home/brian/intel_index/EmbAttSpotter/vlfeat-0.9.20/bin/glnxa64/", "-l:libvl.so"
          ],
      "conditions": [
        [ 'OS=="mac"', {
            "xcode_settings": {
                'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11','-stdlib=libc++'],
                'OTHER_LDFLAGS': ['-stdlib=libc++'],
                'MACOSX_DEPLOYMENT_TARGET': '10.7' }
            }
        ]
      ]
    }
  ]
}
