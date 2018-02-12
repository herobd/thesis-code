import sys
import csv

#0 time, 1 accuracyTrans, 2 pWordsTrans, 3 pWords80_100, 4 pWords60_80, 5 pWords40_60, 6 pWords20_40, 7 pWords0_20, 8 pWords0, 9 pWordsBad, 10 transSent, 11 badTransBatchs, 12 badTransNgram, 13 spotSent, 14 spotAccept, 15 spotReject, 16 spotAutoAccept, 17 spotAutoReject, 18 spottingTime, 19 transTime, 20 badPrunes, 21 accuracyTrans_IV, 22 pWordsTrans_IV, 23 pWords80_100_IV, 24 pWords60_80_IV, 25 pWords40_60_IV, 26 pWords20_40_IV, 27 pWords0_20_IV, 28 pWords0_IV, 29 misTrans
#

out=[['file','% comp.','comp./day','trans sent','true ratio','minutes','acc']]
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
        endString = data[-1][-1]
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

            outRow.append(time*60)
            acc = float(data[endIndex][1])
            outRow.append(acc)
    out.append(outRow)
            


with open('summary.csv', 'wb') as csvfile:
    writer = csv.writer(csvfile, delimiter=',',quotechar='|', quoting=csv.QUOTE_MINIMAL)
    for row in out:
        writer.writerow(row)
