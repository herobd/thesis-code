import sys
import csv

#time,accuracyTrans,pWordsTrans,pWords80_100,pWords60_80,pWords40_60,pWords20_40,pWords0_20,pWords0,pWordsBad,transSent,badTransBatchs,badTransNgram,spotSent,spotAccept,spotReject,spotAutoAccept,spotAutoReject,newExemplarsSpotted,badPrunes,accuracyTrans_IV,pWordsTrans_IV,pWords80_100_IV,pWords60_80_IV,pWords40_60_IV,pWords20_40_IV,pWords0_20_IV,pWords0_IV,misTrans

out=[['file','% comp.','comp./day','trans sent','true ratio']]
usingBad=0

for i in range(1,len(sys.argv)):
    csvfile=sys.argv[i]
    print csvfile
    assert csvfile[-4:]=='.csv'
    outRow = [csvfile[:-4]]
    with open(csvfile, 'rb') as content:
        reader = csv.reader(content, delimiter=',', quotechar='|')
        data=[]
        for row in reader:
            data.append(row)
        #is using new pWordsBad?
        if (data[0][9]=='pWordsBad'):
            usingBad=1
        endString = data[-1][27+usingBad]
        manualEnd = ('MANUAL' in endString)
        blankEnd = ('BLANK' in endString)
        if not manualEnd != blankEnd:
            #outRow.append('nonstandard ending: '+endString)
            outRow[0]+='!NotFinished!'
        if len(data)>1:

            startTime = (int(data[1][0][-8:-6])*60 + int(data[1][0][-5:-3]))*60 + int(data[1][0][-2:])
            endIndex=-1
            if blankEnd:
                while endIndex>1-len(data) and int(data[endIndex][9+usingBad])==0 and int(data[endIndex][12+usingBad])==0 and int(data[endIndex][13+usingBad])==0:
                    endIndex-=1
            if endIndex == 1-len(data):
                endIndex=-1
                    
            endTime = (int(data[endIndex][0][-8:-6])*60 + int(data[endIndex][0][-5:-3]))*60 + int(data[endIndex][0][-2:])
            time = endTime-startTime
            if time<0:
                time += 24*60*60
            endCompletion = float(data[endIndex][2])

            transSent=0
            trueDone=0
            falseDone=0
            for index in range(1,len(data)+endIndex+1):
                transSent += int(data[index][9+usingBad])
                trueDone += int(data[index][12+usingBad])
                falseDone += int(data[index][13+usingBad])

            compsec = endCompletion/(time)
            outRow.append(endCompletion)
            outRow.append(compsec*60*60*24)
            outRow.append(transSent)
            if trueDone+falseDone>0:
                outRow.append(float(trueDone)/float(trueDone+falseDone))
    out.append(outRow)
            


with open('summary.csv', 'wb') as csvfile:
    writer = csv.writer(csvfile, delimiter=',',quotechar='|', quoting=csv.QUOTE_MINIMAL)
    for row in out:
        writer.writerow(row)
