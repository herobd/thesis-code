CXX = g++
LINK = g++
CXX_FLAGS_NO_G =  -std=c++11 -pthread -DNO_NAN -DTEST_MODE #-DGRAPH_SPOTTING_RESULTS #-DDEBUG_AUTO#-ansi -fexceptions -fPIC -fno-omit-frame-pointer -pthread
CXX_FLAGS_NO_G += -DCPU_ONLY

LIBS += -lpthread

LIBS += -lopencv_core -lopencv_highgui -lopencv_imgproc -fopenmp
LIBS += -pthread 

#LIBS += -L/home/brian/intel_index/EmbAttSpotter -lembattspotter -larmadillo
#LIBS += -L/home/brian/intel_index/EmbAttSpotter/vlfeat-0.9.20/bin/glnxa64/ -l:libvl.so
#INCLUDE +=-I. -I/home/brian/intel_index/EmbAttSpotter

##########
#ironsides
###LIBS += -L ../../warp-ctc/build -lwarpctc
###INCLUDE += -I ../../warp-ctc/include
#CXX_FLAGS_NO_G += -DCTC
#LIBS += -L/home/brian/robert_stuff/documentproject/lib -ldtwarp
#INCLUDE += -I/home/brian/robert_stuff/documentproject/src
#
#LIBS += -L ../../brian_caffe/scripts/cnnspp_spotter -lcnnspp_spotter
#INCLUDE += -I ../../brian_caffe/scripts/cnnspp_spotter
#LIBS += -L ../../brian_caffe/build/lib
#INCLUDE +=-I ../../brian_caffe/include
#LIBS += -lcaffe -l:libboost_system.so.1.58.0
#
#CXX_FLAGS_NO_G += -DOPENCV2

##########
#Supercomp, fawkes

CXX_FLAGS_NO_G += -DCTC
LIBS += -L../../docproj/lib -ldtwarp
INCLUDE += -I../../docproj/src

LIBS += -L ../../caffe/scripts/cnnspp_spotter -lcnnspp_spotter
INCLUDE += -I ../../caffe/scripts/cnnspp_spotter
LIBS += -L ../../caffe/build/lib
INCLUDE +=-I ../../caffe/include
LIBS += -lopencv_imgcodecs
LIBS += -lcaffe -l:libboost_system.so.1.59.0 -lglog


CXX_FLAGS =  $(CXX_FLAGS_NO_G) -O3 #-g

PROGRAM_NAME = test

OBJECTS = obj/Global.o \
          obj/CATTSS.o \
          obj/MasterQueue.o \
          obj/TranscribeBatchQueue.o \
          obj/NewExemplarsBatchQueue.o \
          obj/SpottingResults.o \
          obj/ClusterBatcher.o \
          obj/Knowledge.o \
          obj/Lexicon.o \
          obj/maxflow.o \
          obj/graph.o \
          obj/SpottingQueue.o \
          obj/batches.o \
          obj/BatchWraperNewExemplars.o \
          obj/BatchWraperSpottings.o \
          obj/BatchWraperTranscription.o \
          obj/Simulator.o \
          obj/NetSpotter.o \
          obj/CTCWrapper.o \
	  obj/LineQueue.o \
          obj/Web.o
          #obj/tester.o \
          #obj/AlmazanSpotter.o \
          #obj/AlmazanDataset.o \
#          obj/test.o 
          #obj/FacadeSpotter.o \
          #obj/TestQueue.o 
OBJECTSDONTLOAD = obj/Global.o \
          obj/CATTSS.o \
          obj/MasterQueue.o \
          obj/TranscribeBatchQueue.o \
          obj/NewExemplarsBatchQueue.o \
          obj/SpottingResults.o \
          obj/ClusterBatcher.o \
          obj/KnowledgeDONT_LOAD.o \
          obj/Lexicon.o \
          obj/maxflow.o \
          obj/graph.o \
          obj/SpottingQueue.o \
          obj/batches.o \
          obj/BatchWraperNewExemplars.o \
          obj/BatchWraperSpottings.o \
          obj/BatchWraperTranscription.o \
          obj/Simulator.o \
          obj/NetSpotter.o \
          obj/CTCWrapper.o \
	  obj/LineQueue.o \
          obj/Web.o
          #obj/tester.o 
OBJECTS2 = obj/TranscribeBatchQueue2.o \
          obj/NewExemplarsBatchQueue2.o \
          obj/Global2.o \
          obj/MasterQueue2.o \
          obj/SpottingResults2.o \
          obj/ClusterBatcher2.o \
          obj/Knowledge2.o \
          obj/Lexicon2.o \
          obj/maxflow2.o \
          obj/graph2.o \
          obj/SpottingQueue2.o \
          obj/batches2.o \
          obj/CATTSS2.o \
          obj/BatchWraperNewExemplars2.o \
          obj/BatchWraperSpottings2.o \
          obj/BatchWraperTranscription2.o \
          obj/Simulator2.o \
          obj/NetSpotter2.o
          #obj/AlmazanSpotter2.o \
          obj/tester2.o \
          #obj/AlmazanDataset2.o \
#          obj/test2.o 
          #obj/FacadeSpotter2.o \
          #obj/TestQueue2.o 
PREPROCESSEDS = pp/TranscribeBatchQueue.cpp \
          pp/NewExemplarsBatchQueue.cpp \
          pp/Global.cpp \
          pp/MasterQueue.cpp \
          pp/SpottingResults.cpp \
          pp/Knowledge.cpp \
          pp/Lexicon.cpp \
          pp/SpottingQueue.cpp \
          pp/batches.cpp \
          pp/AlmazanSpotter.cpp \
          pp/AlmazanDataset.cpp \
          pp/CATTSS.cpp \
          pp/BatchWraperNewExemplars.cpp \
          pp/BatchWraperSpottings.cpp \
          pp/BatchWraperTranscription.cpp \
          pp/Simulator.cpp 
          #pp/maxflow.cpp \
          pp/tester.cpp \
          #pp/graph.cpp \

bin: $(PROGRAM_NAME)
all: $(PROGRAM_NAME) simulation
tr: train
data: makeData
getDone: getDone
#viewer: viewer
sim: simulation
sim2: simulation2
sim3: simulationNoWait
sim4: simulationExemplars
pp: $(PREPROCESSEDS)
expand: removeinc addinc simulation2
addinc: $(PREPROCESSEDS)
	- perl -pi -e 's/\/\/AUTO!//g' pp/*.cpp 
	- perl -pi -e 's/(#include <b64\/encode\.h>)/#define BUFFERSIZE 65536\r\1/g' pp/*.cpp 

removeinc:
	- cp *.cpp noinc/
	- cp *.h noinc/
	- perl -pi -e 's/^(#include.*<.*>.*)$$/\/\/AUTO!\1/' noinc/*
	- perl -pi -e 's/^(#include.*opencv.*)$$/\/\/AUTO!\1/' noinc/* 
	- perl -pi -e 's/^(#include.*maxflow.*)$$/\/\/AUTO!\1/' noinc/* 
	- perl -pi -e 's/^(#include.*vl_?feat.*)$$/\/\/AUTO!\1/' noinc/* 
	- perl -pi -e 's/^(#include.*semaphore.*)$$/\/\/AUTO!\1/' noinc/* 
	- perl -pi -e 's/^(#include.*embattspotter.*)$$/\/\/AUTO!\1/' noinc/* 
	- perl -pi -e 's/^(#include.*cnnspp_spotter.*)$$/\/\/AUTO!\1/' noinc/* 
	- perl -pi -e 's/^(#include.*dataset.*)$$/\/\/AUTO!\1/' noinc/* 

clean:
	- rm  $(PROGRAM_NAME)
	- rm  train
	- rm  makeData
	- rm  simulation
	- rm  obj/*.o
	- rm  getDone
	- rm  viewer

cleanex:
	- rm  simulation2
	- rm  pp/*.cpp
	- rm  obj/*2.o

$(PROGRAM_NAME):  obj/test.o $(OBJECTS)
	$(LINK) $(CXX_FLAGS) -o $(PROGRAM_NAME) obj/test.o $(OBJECTS) $(LIBS)

simulation: $(OBJECTS) simulation.cpp
	$(LINK) $(CXX_FLAGS) -o simulation simulation.cpp $(INCLUDE) $(OBJECTS) $(LIBS)

simulation2: $(OBJECTS) simulation.cpp
	$(LINK) $(CXX_FLAGS) -o simulation2 simulation.cpp $(INCLUDE) $(OBJECTS) $(LIBS)

simulationNoWait: $(OBJECTS) simulation.cpp
	$(LINK) $(CXX_FLAGS) -o simulationNoWait simulation.cpp $(INCLUDE) $(OBJECTS) $(LIBS)

simulationExemplars: $(OBJECTS) simulation.cpp
	$(LINK) $(CXX_FLAGS) -o simulationExemplars simulation.cpp $(INCLUDE) $(OBJECTS) $(LIBS)

train: $(OBJECTS)
	$(LINK) $(CXX_FLAGS) -o train train.cpp $(INCLUDE) $(OBJECTS) $(LIBS)

getDone: $(OBJECTSDONTLOAD) getDone.cpp
	$(LINK) $(CXX_FLAGS) -DDONT_LOAD -o getDone getDone.cpp $(INCLUDE) $(OBJECTSDONTLOAD) $(LIBS)
	
viewer: $(OBJECTS) viewer.cpp
	$(LINK) $(CXX_FLAGS) -o viewer viewer.cpp $(INCLUDE) $(OBJECTS) $(LIBS)
	
makeData: $(OBJECTS)
	$(LINK) $(CXX_FLAGS) -o makeData makeData.cpp $(INCLUDE) $(OBJECTS) $(LIBS)
	
obj/tester.o: tester.cpp tester.h CATTSS.h Simulator.h BatchWraper.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/tester.o tester.cpp $(LIBS)

obj/Global.o: Global.h Global.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Global.o Global.cpp $(LIBS)

obj/CATTSS.o: CATTSS.h CATTSS.cpp MasterQueue.h Lexicon.h SpottingQueue.h Knowledge.h spotting.h BatchWraper.h Web.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/CATTSS.o CATTSS.cpp $(LIBS)

obj/test.o: test.cpp MasterQueue.h TestQueue.h MasterQueue.h Knowledge.h Spotter.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/test.o test.cpp $(LIBS)

obj/Simulator.o: Simulator.h Simulator.cpp CATTSS.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Simulator.o Simulator.cpp $(LIBS)

obj/TranscribeBatchQueue.o: TranscribeBatchQueue.h TranscribeBatchQueue.cpp Knowledge.h CorpusRef.h PageRef.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/TranscribeBatchQueue.o TranscribeBatchQueue.cpp $(LIBS)

obj/NewExemplarsBatchQueue.o: NewExemplarsBatchQueue.h NewExemplarsBatchQueue.cpp CorpusRef.h PageRef.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/NewExemplarsBatchQueue.o NewExemplarsBatchQueue.cpp $(LIBS)

#obj/AlmazanSpotter.o: AlmazanSpotter.h AlmazanSpotter.cpp AlmazanDataset.h
#	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/AlmazanSpotter.o AlmazanSpotter.cpp $(LIBS)

#obj/AlmazanDataset.o: AlmazanDataset.h AlmazanDataset.cpp
#	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/AlmazanDataset.o AlmazanDataset.cpp $(LIBS)

obj/NetSpotter.o: NetSpotter.h NetSpotter.cpp Spotter.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/NetSpotter.o NetSpotter.cpp $(LIBS)

obj/SpottingQueue.o: SpottingQueue.h SpottingQueue.cpp Knowledge.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/SpottingQueue.o SpottingQueue.cpp $(LIBS)

obj/batches.o: batches.h batches.cpp WordBackPointer.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/batches.o batches.cpp $(LIBS)

#obj/SpottingQuery.o: SpottingQuery.h SpottingQuery.cpp
#	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/SpottingQuery.o SpottingQuery.cpp $(LIBS)


obj/MasterQueue.o: MasterQueue.h MasterQueue.cpp SpottingResults.h Global.h ClusterBatcher.h BatchWraper.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/MasterQueue.o MasterQueue.cpp $(LIBS)

obj/SpottingResults.o: SpottingResults.h SpottingResults.cpp Batcher.h BatchWraper.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/SpottingResults.o SpottingResults.cpp $(LIBS)
	
obj/ClusterBatcher.o: ClusterBatcher.h ClusterBatcher.cpp Batcher.h BatchWraper.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/ClusterBatcher.o ClusterBatcher.cpp $(LIBS)
	
obj/TestQueue.o: TestQueue.h TestQueue.cpp SpottingResults.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/TestQueue.o TestQueue.cpp $(LIBS)
	
obj/Knowledge.o: Knowledge.h Knowledge.cpp Lexicon.h Global.h CorpusRef.h PageRef.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Knowledge.o Knowledge.cpp $(LIBS)
	
obj/KnowledgeDONT_LOAD.o: Knowledge.h Knowledge.cpp Lexicon.h Global.h CorpusRef.h PageRef.h
	$(CXX) -c $(CXX_FLAGS) -DDONT_LOAD $(INCLUDE) -o obj/KnowledgeDONT_LOAD.o Knowledge.cpp $(LIBS)
	
obj/Lexicon.o: Lexicon.h Lexicon.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Lexicon.o Lexicon.cpp $(LIBS)

obj/CTCWrapper.o: CTCWrapper.h CTCWrapper.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/CTCWrapper.o CTCWrapper.cpp $(LIBS)

obj/LineQueue.o: LineQueue.h LineQueue.cpp PsuedoWordBackPointer.h batches.h Knowledge.h BatchWraperTranscription.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/LineQueue.o LineQueue.cpp $(LIBS)

obj/Web.o: Web.h Web.cpp Global.h spotting.h Knowledge.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Web.o Web.cpp $(LIBS)

obj/Spotter.o: Spotter.h Spotter.cpp MasterQueue.h Knowledge.h 
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Spotter.o Spotter.cpp $(LIBS)

obj/FacadeSpotter.o: Spotter.h FacadeSpotter.cpp MasterQueue.h Knowledge.h FacadeSpotter.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/FacadeSpotter.o FacadeSpotter.cpp $(LIBS)

obj/BatchWraperTranscription.o: BatchWraperTranscription.h BatchWraperTranscription.cpp spotting.h BatchWraper.h batches.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/BatchWraperTranscription.o BatchWraperTranscription.cpp $(LIBS)

obj/BatchWraperNewExemplars.o: BatchWraperNewExemplars.h BatchWraperNewExemplars.cpp BatchWraper.h batches.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/BatchWraperNewExemplars.o BatchWraperNewExemplars.cpp $(LIBS)

obj/BatchWraperSpottings.o: BatchWraperSpottings.h BatchWraperSpottings.cpp spotting.h BatchWraper.h batches.h
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/BatchWraperSpottings.o BatchWraperSpottings.cpp $(LIBS)

obj/maxflow.o: maxflow/maxflow.cpp maxflow/graph.h maxflow/block.h  maxflow/instances.inc
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/maxflow.o maxflow/maxflow.cpp

obj/graph.o: maxflow/graph.cpp maxflow/graph.h \
                maxflow/block.h \
                maxflow/instances.inc
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/graph.o maxflow/graph.cpp


############PP##################
pp/tester.cpp: noinc/tester.cpp noinc/tester.h noinc/CATTSS.h noinc/Simulator.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/tester.cpp noinc/tester.cpp $(LIBS)

pp/test.cpp: noinc/test.cpp noinc/MasterQueue.h noinc/TestQueue.h noinc/MasterQueue.h noinc/Knowledge.h noinc/Spotter.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/test.cpp noinc/test.cpp $(LIBS)

pp/Simulator.cpp: noinc/Simulator.h noinc/Simulator.cpp noinc/CATTSS.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/Simulator.cpp noinc/Simulator.cpp $(LIBS)

pp/TranscribeBatchQueue.cpp: noinc/TranscribeBatchQueue.h noinc/TranscribeBatchQueue.cpp noinc/Knowledge.h noinc/CorpusRef.h noinc/PageRef.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/TranscribeBatchQueue.cpp noinc/TranscribeBatchQueue.cpp $(LIBS)

pp/NewExemplarsBatchQueue.cpp: noinc/NewExemplarsBatchQueue.h noinc/NewExemplarsBatchQueue.cpp noinc/CorpusRef.h noinc/PageRef.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/NewExemplarsBatchQueue.cpp noinc/NewExemplarsBatchQueue.cpp $(LIBS)

pp/AlmazanSpotter.cpp: noinc/AlmazanSpotter.h noinc/AlmazanSpotter.cpp noinc/AlmazanDataset.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/AlmazanSpotter.cpp noinc/AlmazanSpotter.cpp $(LIBS)

pp/AlmazanDataset.cpp: noinc/AlmazanDataset.h noinc/AlmazanDataset.cpp
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/AlmazanDataset.cpp noinc/AlmazanDataset.cpp $(LIBS)

pp/SpottingQueue.cpp: noinc/SpottingQueue.h noinc/SpottingQueue.cpp noinc/Knowledge.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/SpottingQueue.cpp noinc/SpottingQueue.cpp $(LIBS)

pp/batches.cpp: noinc/batches.h noinc/batches.cpp noinc/WordBackPointer.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/batches.cpp noinc/batches.cpp $(LIBS)

#pp/SpottingQuery.cpp: noinc/SpottingQuery.h noinc/SpottingQuery.cpp
#	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/SpottingQuery.cpp noinc/SpottingQuery.cpp $(LIBS)

pp/Global.cpp: noinc/Global.h noinc/Global.cpp
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/Global.cpp noinc/Global.cpp $(LIBS)

pp/CATTSS.cpp: noinc/CATTSS.h noinc/CATTSS.cpp noinc/MasterQueue.h noinc/Lexicon.h noinc/SpottingQueue.h noinc/Knowledge.h noinc/spotting.h noinc/BatchWraper.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/CATTSS.cpp noinc/CATTSS.cpp $(LIBS)

pp/MasterQueue.cpp: noinc/MasterQueue.h noinc/MasterQueue.cpp noinc/SpottingResults.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/MasterQueue.cpp noinc/MasterQueue.cpp $(LIBS)

pp/SpottingResults.cpp: noinc/SpottingResults.h noinc/SpottingResults.cpp 
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/SpottingResults.cpp noinc/SpottingResults.cpp $(LIBS)
	
pp/TestQueue.cpp: noinc/TestQueue.h noinc/TestQueue.cpp noinc/SpottingResults.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/TestQueue.cpp noinc/TestQueue.cpp $(LIBS)
	
pp/Knowledge.cpp: noinc/Knowledge.h noinc/Knowledge.cpp noinc/Lexicon.h noinc/Global.h noinc/CorpusRef.h noinc/PageRef.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/Knowledge.cpp noinc/Knowledge.cpp $(LIBS)
	
pp/Lexicon.cpp: noinc/Lexicon.h noinc/Lexicon.cpp
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/Lexicon.cpp noinc/Lexicon.cpp $(LIBS)

pp/Spotter.cpp: noinc/Spotter.h noinc/Spotter.cpp noinc/MasterQueue.h noinc/Knowledge.h 
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/Spotter.cpp noinc/Spotter.cpp $(LIBS)

pp/FacadeSpotter.cpp: noinc/Spotter.h noinc/FacadeSpotter.cpp noinc/MasterQueue.h noinc/Knowledge.h noinc/FacadeSpotter.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/FacadeSpotter.cpp noinc/FacadeSpotter.cpp $(LIBS)

pp/BatchWraperTranscription.cpp: noinc/BatchWraperTranscription.h noinc/BatchWraperTranscription.cpp noinc/spotting.h noinc/BatchWraper.h noinc/batches.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/BatchWraperTranscription.cpp noinc/BatchWraperTranscription.cpp $(LIBS)

pp/BatchWraperNewExemplars.cpp: noinc/BatchWraperNewExemplars.h noinc/BatchWraperNewExemplars.cpp noinc/BatchWraper.h noinc/batches.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/BatchWraperNewExemplars.cpp noinc/BatchWraperNewExemplars.cpp $(LIBS)

pp/BatchWraperSpottings.cpp: noinc/BatchWraperSpottings.h noinc/BatchWraperSpottings.cpp noinc/spotting.h noinc/BatchWraper.h noinc/batches.h
	$(CXX) -E -C -P $(CXX_FLAGS_NO_G) $(INCLUDE) -o pp/BatchWraperSpottings.cpp noinc/BatchWraperSpottings.cpp $(LIBS)

#pp/maxflow.cpp: noinc/maxflow/maxflow.cpp maxflow/graph.h maxflow/block.h  maxflow/instances.inc
#	$(CXX) -E $(CXX_FLAGS) $(INCLUDE) -o pp/maxflow.cpp noinc/maxflow/maxflow.cpp
#
#pp/graph.cpp: noinc/maxflow/graph.cpp maxflow/graph.h \
#                maxflow/block.h \
#                maxflow/instances.inc
#	$(CXX) -E $(CXX_FLAGS) $(INCLUDE) -o pp/graph.cpp noinc/maxflow/graph.cpp
#############################

##################OBJ2#####################################
obj/tester2.o: pp/tester.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/tester2.o pp/tester.cpp $(LIBS)

obj/test2.o: pp/test.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/test2.o pp/test.cpp $(LIBS)

obj/Simulator2.o: pp/Simulator.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Simulator2.o pp/Simulator.cpp $(LIBS)

obj/TranscribeBatchQueue2.o: pp/TranscribeBatchQueue.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/TranscribeBatchQueue2.o pp/TranscribeBatchQueue.cpp $(LIBS)

obj/NewExemplarsBatchQueue2.o: pp/NewExemplarsBatchQueue.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/NewExemplarsBatchQueue2.o pp/NewExemplarsBatchQueue.cpp $(LIBS)

obj/AlmazanSpotter2.o: pp/AlmazanSpotter.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/AlmazanSpotter2.o pp/AlmazanSpotter.cpp $(LIBS)

obj/AlmazanDataset2.o: pp/AlmazanDataset.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/AlmazanDataset2.o pp/AlmazanDataset.cpp $(LIBS)

obj/SpottingQueue2.o: pp/SpottingQueue.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/SpottingQueue2.o pp/SpottingQueue.cpp $(LIBS)

obj/batches2.o: pp/batches.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/batches2.o pp/batches.cpp $(LIBS)

#obj/SpottingQuery2.o: SpottingQuery.i
#	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/SpottingQuery2.o SpottingQuery.i $(LIBS)

obj/Global2.o: pp/Global.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Global2.o pp/Global.cpp $(LIBS)

obj/CATTSS2.o: pp/CATTSS.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/CATTSS2.o pp/CATTSS.cpp $(LIBS)

obj/MasterQueue2.o: pp/MasterQueue.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/MasterQueue2.o pp/MasterQueue.cpp $(LIBS)

obj/SpottingResults2.o: pp/SpottingResults.cpp 
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/SpottingResults2.o pp/SpottingResults.cpp $(LIBS)
	
obj/TestQueue2.o: pp/TestQueue.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/TestQueue2.o pp/TestQueue.cpp $(LIBS)
	
obj/Knowledge2.o: pp/Knowledge.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Knowledge2.o pp/Knowledge.cpp $(LIBS)
	
obj/Lexicon2.o: pp/Lexicon.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Lexicon2.o pp/Lexicon.cpp $(LIBS)

obj/Spotter2.o: pp/Spotter.cpp 
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/Spotter2.o pp/Spotter.cpp $(LIBS)

obj/FacadeSpotter2.o: pp/FacadeSpotter.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/FacadeSpotter2.o pp/FacadeSpotter.cpp $(LIBS)

obj/BatchWraperTranscription2.o: pp/BatchWraperTranscription.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/BatchWraperTranscription2.o pp/BatchWraperTranscription.cpp $(LIBS)

obj/BatchWraperNewExemplars2.o: pp/BatchWraperNewExemplars.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/BatchWraperNewExemplars2.o pp/BatchWraperNewExemplars.cpp $(LIBS)

obj/BatchWraperSpottings2.o: pp/BatchWraperSpottings.cpp
	$(CXX) -c $(CXX_FLAGS) $(INCLUDE) -o obj/BatchWraperSpottings2.o pp/BatchWraperSpottings.cpp $(LIBS)

obj/maxflow2.o: obj/maxflow.o 
	- cp obj/maxflow.o obj/maxflow2.o
	 

obj/graph2.o: obj/graph.o
	- cp obj/graph.o obj/graph2.o
