import sys
import csv

out=[['file','% comp.','comp./day','trans sent','true ratio']]

for i in range(1,len(sys.argv)):
    csvfile=sys.argv[i]
    assert csvfile[-4:]=='.csv'
    outRow = [csvfile[:-4]]
    with open(csvfile, 'rb') as content:
        reader = csv.reader(content, delimiter=',', quotechar='|')
        data=[]
        for row in reader:
            data.append(row)
        endString = data[-1][27]
        manualEnd = ('MANUAL' in endString)
        blankEnd = ('BLANK' in endString)
        if not manualEnd != blankEnd:
            outRow.append('nonstandard ending: '+endString)
        elif len(data)>1:

            startTime = (int(data[1][0][-8:-6])*60 + int(data[1][0][-5:-3]))*60 + int(data[1][0][-2:])
            endIndex=-1
            if blankEnd:
                while int(data[endIndex][9])==0 and int(data[endIndex][12])==0 and int(data[endIndex][13])==0:
                    endIndex-=1
                    
            endTime = (int(data[endIndex][0][-8:-6])*60 + int(data[endIndex][0][-5:-3]))*60 + int(data[endIndex][0][-2:])
            time = endTime-startTime
            if time<0:
                time += 24*60*60
            endCompletion = float(data[endIndex][2])

            transSent=0
            trueDone=0
            falseDone=0
            for index in range(1,len(data)+endIndex+1):
                transSent += int(data[index][9])
                trueDone += int(data[index][12])
                falseDone += int(data[index][13])

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
