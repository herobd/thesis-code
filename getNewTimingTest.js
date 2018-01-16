const assert = require('assert');
var fs = require('fs');
const readline = require('readline');


//var datanames=['BENTHAMfancy','BENTHAMcluster_step','NAMESfancy','NAMEScluster_step'];
var datanames=['BENTHAMphoc_trans'];//,'NAMESphoc_trans'];
var lineDatanames=['BENTHAMline'];//,'NAMESline'];
//var datanames=['NAMESphoc_trans'];
//var lineDatanames=['NAMESline'];


function shuffle(array) { //from: http://stackoverflow.com/a/2450976/1018830
  var currentIndex = array.length, temporaryValue, randomIndex;

  // While there remain elements to shuffle...
  while (0 !== currentIndex) {

    // Pick a remaining element...
    randomIndex = Math.floor(Math.random() * currentIndex);
    currentIndex -= 1;

    // And swap it with the current element.
    temporaryValue = array[currentIndex];
    array[currentIndex] = array[randomIndex];
    array[randomIndex] = temporaryValue;
  }

  return array;
}

function findParamsManNew(err,manTimingInstances,dataname) {
    console.log('----MAN----'+dataname);
    console.log(manTimingInstances.length+' instances');
    if (manTimingInstances.length==0) {
        console.log("maxTimingInstances is empty");
        return;
    }
    var meanTime=0.0;

    var minTime=999999;
    var maxTime=-1;

    var meanAcc=0.0;
    var numWords=0;


    const rl = readline.createInterface({
          input: process.stdin,
            output: process.stdout
    });

    var allCSV='acc,time,numChar,prev,user\n';
    var finishLine = function(i,inst) {

        if (inst != null) {
            meanTime+= inst.time;
            meanAcc+= inst.accuracy*inst.wordC;
            numWords += inst.wordC;

            allCSV+=inst.accuracy+','+inst.time+','+inst.numChar+','+(inst.prev?1:0)+','+inst.user+'\n';
        }

        if (i<manTimingInstances.length-1)
            recur(i+1);
        else {
            //write
            console.log(dataname+' done');
            console.log(dataname+' acc (by word): '+meanAcc/numWords);
            var stream = fs.createWriteStream("userNew/man_all_"+dataname+".csv");
            stream.once('open', function(fd) {
                stream.write(allCSV);
                stream.close();
            });
            rl.close();
        }
    }
    var recur = function(i) {
        var inst = manTimingInstances[i];
        var label=inst.trans.toLowerCase().trim().replace(/\s/g,' ').replace(/[^ \w]/g,'').replace(/ +/g,' ').split(' ');
        inst.wordC = label.length;
        if (inst.accuracy<1.0)
        {
            console.log(dataname+" acc: "+inst.accuracy);
            console.log(dataname+' gt:    '+inst.gt);
            console.log(dataname+' trans: '+inst.trans);
            console.log(dataname+' line: '+inst.line);
            rl.question(dataname+' new gt( ! ): ', function(answer) {
                  //console.log('Thank you for your valuable feedback: '+answer);

                if (answer.length>0) {
                    if (answer[0]=='!') {
                        console.log(dataname+' skipped');
                        inst=null;
                    } else {
                        var gt=answer.toLowerCase().trim().replace(/\s/g,' ').replace(/[^ \w]/g,'').replace(/ +/g,' ').split(' ');
                        //var label above
                        //Dymanic programming for word alignment
                            var dtmap = [];
                            dtmap[0]=(label[0]==gt[0]?0:1);
                            for (var gtP=1; gtP<gt.length; gtP++) {
                                dtmap[(gtP)] = gtP;
                            }
                            for (var labelP=1; labelP<label.length; labelP++) {
                                dtmap[(labelP)*gt.length] = labelP;
                            }
                            for (var gtP=1; gtP<gt.length; gtP++) {
                                for (var labelP=1; labelP<label.length; labelP++) {
                                    var stepScore = dtmap[(gtP-1)+(labelP-1)*gt.length] + (label[labelP]==gt[gtP]?0:1);
                                    var skipGtScore = dtmap[(gtP)+(labelP-1)*gt.length] + 1;
                                    var skipLabelScore = dtmap[(gtP-1)+(labelP)*gt.length] + 1;
                                    dtmap[(gtP)+(labelP)*gt.length] = Math.min(stepScore,skipGtScore,skipLabelScore);
                                }
                            }
                            var score = dtmap[gt.length*label.length-1];
                            var accuracy = Math.max(0,gt.length-score)/(0.0+gt.length);
                        inst.accuracy=accuracy;
                        console.log(dataname+' new acc: '+accuracy);
                    }
                }
                finishLine(i,inst);
            });
        } else {
            finishLine(i,inst);
        }
    }
    //commence
    recur(0);
}
//time = c + numChars*n + numPoss*p + position*i + prevTrans*t + 
function findParamsMan(err,manTimingInstances,dataname) {
    console.log('----MAN----'+dataname);
    console.log(manTimingInstances.length+' instances');
    if (manTimingInstances.length==0) {
        console.log("maxTimingInstances is empty");
        return;
    }
    var meanTime=0.0;

    var minTime=999999;
    var maxTime=-1;

    var meanAcc=0.0;


    var allCSV='acc,time,numChar,prev,user\n';
    for (var inst of manTimingInstances)
    {
        meanTime+= inst.time;
        meanAcc+= inst.accuracy;

        allCSV+=inst.accuracy+','+inst.time+','+inst.numChar+','+(inst.prev?1:0)+','+inst.user+'\n';
    }
    meanTime /= manTimingInstances.length;
    meanAcc /= manTimingInstances.length;
    
    console.log('TIME');
    console.log('all  : '+meanTime);
    var stdTime=0.0;
    for (var inst of manTimingInstances)
    {
        stdTime += (inst.time-meanTime)*(inst.time-meanTime);
    }
    stdTime = Math.sqrt(stdTime/manTimingInstances.length);
    console.log('std time: '+stdTime);

    console.log('\nACC');
    console.log('all  : '+meanAcc);
    var stream = fs.createWriteStream("user/man_"+dataname+".csv");
    stream.once('open', function(fd) {
        stream.write(allCSV);
        stream.close();

    });
    console.log('---done man---');
    return;
}

function findParamsTransNew(err,transTimingInstances,dataname) {
    console.log('---TRANS----'+dataname);
    console.log(transTimingInstances.length+' instances');
    if (transTimingInstances.length==0) {
        console.log("transTimingInstances is empty");
        return;
    }
    //overall time and acc and error-types
    var allCount=0.0;
    var allTime=0;
    var allAcc=0;
    var allErrorNone=0;
    var allErrorWord=0;
    //when-present time and acc and error-types
    var availCount=0.0;
    var availTime=0;
    var availAcc=0;
    var availErrorNone=0;
    var availErrorWord=0;
    //when-not-present time and acc (one error type)
    var notCount=0.0;
    var notTime=0;
    var notAcc=0;

    var allCSV='acc,time,numPoss,position,prev,avail,bad,error,user\n';
    var availCSV='acc,time,numPoss,position,prev,user';
    var notCSV='acc,time,numPoss,position,prev,user';
    for (var inst of transTimingInstances)
    {
        position = inst.position==-1?-10:inst.position;
        //if (inst.time<minTime)
        //    minTime=inst.time;
        //if (inst.time<maxTime)
        //    maxTime=inst.time;

        allCount++;
        allAcc+= inst.accuracy;
        allTime+= inst.time;

        if (inst.position>-1) {
            availCount++;
            availAcc+=inst.accuracy;
            availTime+=inst.time;
            availCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+inst.user+'\n';
        }
        else {
            notCount++;
            notAcc+=inst.accuracy;
            notTime+=inst.time;
            notCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+inst.user+'\n';
        }

        if (inst.accuracy<1) {
            console.log(dataname+' error trans: '+inst.trans+'   gt: '+inst.gt)
            if (inst.trans=="$ERROR$") {
                allErrorNone++;
                if (inst.position>-1)
                    availErrorNone++;
            } else {
                allErrorWord++;
                if (inst.position>-1)
                    availErrorWord++;
            }
        }
                

        allCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+(inst.position>-1?1:0)+','+(inst.bad?1:0)+','+(inst.error?1:0)+','+inst.user+'\n';
    }
    console.log(dataname+' ALL');
    console.log(dataname+'  time: '+(allTime/allCount));
    console.log(dataname+'  acc: '+(allAcc/allCount));
    console.log(dataname+'  error-none: '+(allErrorNone/allCount));
    console.log(dataname+'  error-word: '+(allErrorWord/allCount));
    console.log(dataname+' TRANS AVAIL');
    console.log(dataname+'  time: '+(availTime/availCount));
    console.log(dataname+'  acc: '+(availAcc/availCount));
    console.log(dataname+'  error-none: '+(availErrorNone/availCount));
    console.log(dataname+'  error-word: '+(availErrorWord/availCount));
    console.log(dataname+' NOT AVAIL');
    console.log(dataname+'  time: '+(notTime/notCount));
    console.log(dataname+'  acc: '+(notAcc/notCount));
    var stream = fs.createWriteStream("userNew/trans_all_"+dataname+".csv");
    stream.once('open', function(fd) {
        stream.write(allCSV);
        stream.close();

        stream = fs.createWriteStream("userNew/trans_avail_"+dataname+".csv");
        stream.once('open', function(fd) {
            stream.write(availCSV);
            stream.close();

            stream = fs.createWriteStream("userNew/trans_not_"+dataname+".csv");
            stream.once('open', function(fd) {
                stream.write(notCSV);
                stream.close();
            });
        });
    });
}

//time = c + numChars*n + numPoss*p + position*i + prevTrans*t + 
function findParamsTrans(err,transTimingInstances,dataname) {
    console.log('---TRANS----'+dataname);
    console.log(transTimingInstances.length+' instances');
    if (transTimingInstances.length==0) {
        console.log("transTimingInstances is empty");
        return;
    }
    var meanTime=0.0;
    var meanTimeAvail=0.0;
    var meanTimeBad=0.0;
    var meanTimeError=0.0;
    var meanTimeNotAvail=0.0;

    var minTime=999999;
    var maxTime=-1;

    var meanAcc=0.0;
    var meanAccAvail=0.0;
    var meanAccBad=0.0;
    var meanAccError=0.0;
    var meanAccNotAvail=0.0;

    var countAvail=0;
    var countBad=0;
    var countError=0;

    var allCSV='acc,time,numPoss,position,prev,avail,bad,error,user\n';
    var availCSV='acc,time,numPoss,position,prev,user';
    var notCSV='acc,time,numPoss,position,prev,user';
    var badCSV='';
    var errorCSV='';

    var misclassified=0;
    var misclassifiedCount=0;
    var misclassified2=0;
    var misclassified2Count=0;
    //var avgN={};
    //var countN={};
    for (var inst of transTimingInstances)
    {
        position = inst.position==-1?10:inst.position;
        meanTime += inst.time;
        if (inst.time<minTime)
            minTime=inst.time;
        if (inst.time<maxTime)
            maxTime=inst.time;

        meanAcc+= inst.accuracy;

        allCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+(inst.position>-1?1:0)+','+(inst.bad?1:0)+','+(inst.error?1:0)+','+inst.user+'\n';
        if (inst.position>-1) {
            meanAccAvail+= inst.accuracy;
            meanTimeAvail += inst.time;
            countAvail+=1;
            availCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+inst.user+'\n';
            misclassifiedCount++;
            misclassified+=inst.badTrans;
        }
        else {
            misclassified2Count++;
            misclassified2+=inst.badTrans;
            meanTimeNotAvail+=inst.time;
            meanAccNotAvail += inst.accuracy;
            notCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+inst.user+'\n';
        }
        if (inst.bad) {
            meanAccBad+= inst.accuracy;
            meanTimeBad += inst.time;
            //console.log('Bad time: '+inst.time);
            countBad+=1;
            badCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+inst.user+'\n';
        }
        if (inst.error) {
            meanAccError+= inst.accuracy;
            meanTimeError += inst.time;
            //console.log('Error time: '+inst.time);
            countError+=1;
            errorCSV+=inst.accuracy+','+inst.time+','+inst.numPoss+','+position+','+(inst.prev?1:0)+','+inst.user+'\n';
        }
        //if (!avgN.hasOwnProperty(inst.n)) {
        //    avgN[inst.n]=0;
        //    countN[inst.n]=0.0;
        //}
        //avgN[inst.n]+=inst.time;
        //countN[inst.n]+=1.0;
    }
    console.log(misclassified+' / '+misclassifiedCount+' = '+(misclassified/(0.0+misclassifiedCount)));
    console.log(misclassified2+' / '+misclassified2Count+' = '+(misclassified2/(0.0+misclassified2Count)));
    return;
    meanTime /= transTimingInstances.length;
    meanAcc /= transTimingInstances.length;
    
    meanTimeAvail /= countAvail;
    meanAccAvail /= countAvail;
    meanTimeNotAvail /= transTimingInstances.length-countAvail;
    meanAccNotAvail /= transTimingInstances.length-countAvail;

    meanTimeBad /= countBad;
    meanAccBad /= countBad;

    meanTimeError /= countError;
    meanAccError /= countError;
    console.log('tot:'+transTimingInstances.length+' avail:'+countAvail+' bad:'+countBad+' error:'+countError);
    console.log('TIME');
    console.log('all  : '+meanTime);
    console.log('avail: '+meanTimeAvail);
    console.log('not avail: '+meanTimeNotAvail);
    console.log('bad  : '+meanTimeBad);
    console.log('error: '+meanTimeError);
    //for (var n in avgN) {
    //    console.log(n+': '+(avgN[n]/countN[n]));
    //}
    var stdTime=0.0;
    for (var inst of transTimingInstances)
    {
        stdTime += (inst.time-meanTime)*(inst.time-meanTime);
    }
    stdTime = Math.sqrt(stdTime/transTimingInstances.length);
    console.log('std time: '+stdTime);

    console.log('\nACC');
    console.log('all  : '+meanAcc);
    console.log('avail: '+meanAccAvail);
    console.log('not avail: '+meanAccNotAvail);
    console.log('bad  : '+meanAccBad);
    console.log('error: '+meanAccError);
    var stream = fs.createWriteStream("user/timing_all_"+dataname+".csv");
    stream.once('open', function(fd) {
        stream.write(allCSV);
        stream.close();

        stream = fs.createWriteStream("user/timing_avail_"+dataname+".csv");
        stream.once('open', function(fd) {
            stream.write(availCSV);
            stream.close();

            stream = fs.createWriteStream("user/timing_not_"+dataname+".csv");
            stream.once('open', function(fd) {
                stream.write(notCSV);
                stream.close();

                stream = fs.createWriteStream("user/timing_error_"+dataname+".csv");
                stream.once('open', function(fd) {
                    stream.write(errorCSV);
                    stream.close();
                });
            });
        });
    });

    return;
    ////////////////////////////////////////////////////////
    var c=meanTime+1.5*stdTime;
    var t=-1*stdTime*1.5;
    var s=stdTime*1.0;
    var p=-1*stdTime*0.75;
    var a=-1*stdTime*0.5;
    console.log('c:'+c+' t:'+t+' s:'+s+' p:'+p+' a:'+a);
    //assert(false);

    var trainingIndex=1;
    var slr=[0.00005, 0.00005, 0.00005, 0.00005, 0.00005];
    for (var i=0; i<3200; i++)
    {
        var lr=slr.slice(0);
        lr[trainingIndex]=(lr[trainingIndex]*2); //(0.0002)/(Math.min(220,i)/8.0+1);
        for (var ii=0; ii<60; ii++) {
            transTimingInstances = shuffle(transTimingInstances);
            var avgDif=0;
            for (inst of transTimingInstances)
            {
                //console.log(inst);
                //console.log(inst.time+' - ('+c+' + '+inst.numT+'*'+t+' + '+inst.numSkip+'*'+s+' + ('+inst.prevSame+'?1:0)*'+p+' + '+inst.accuracy+'*'+a+')');
                var dif = inst.time - (c + inst.numT*t + 0/*inst.numSkip*s*/ + (inst.prevSame?1:0)*p + inst.accuracy*a);
                c += dif*lr[0];
                t += dif*(inst.numT*lr[1]);
                //s += dif*(inst.numSkip*lr[2]);
                p += dif*((inst.prevSame?1:0)*lr[3]);
                a += dif*(inst.accuracy*lr[4]);
                avgDif+=dif*dif;
                //console.log('dif: '+dif);
                //assert(!isNaN(c));
            }
            //assert(false);
            //console.log(lr);
            roundDif = (avgDif/transTimingInstances.length);
            console.log('dif: '+roundDif+'\n');
            if (Math.abs(roundDif) < 10)
            {
                var avgT=0;
                for (inst of transTimingInstances)
                {
                    avgT += (c + inst.numT*t + 0/*inst.numSkip*s*/ + (inst.prevSame?1:0)*p + inst.accuracy*a);
                }
                console.log('Real mean: '+meanTime);
                console.log('Est mean:  '+avgT/transTimingInstances.length);
                if (Math.abs(avgT/transTimingInstances.length - meanTime) <50) {
                    i=1000000;
                    break;
                }
            }
            for (var j=0; j<lr.length; j++)
                lr[j]*=0.9;
        }
        //console.log('i:'+i+'      c:'+c+' t:'+t+' s:'+s+' p:'+p+' a:'+a);
        console.log('i:'+i+'      c:'+c+' t:'+t+' p:'+p+' a:'+a);
        
        trainingIndex = (trainingIndex+1)%lr.length;
        for (var j=0; j<slr.length; j++)
            slr[j]-=0.00000001;
    }
    var avgT=0;
    for (inst of transTimingInstances)
    {
        avgT += (c + inst.numT*t + 0/*inst.numSkip*s*/ + (inst.prevSame?1:0)*p + inst.accuracy*a);
    }
    console.log('Real std:  '+stdTime);
    console.log('Real mean: '+meanTime);
    console.log('Est mean:  '+avgT/transTimingInstances.length);

    console.log('---done trans---');
}

function findParams2(err,spottingTimingInstances,dataname) {
    console.log('---SPOTTINGS---'+dataname);
    console.log(spottingTimingInstances.length+' instances');
    assert(spottingTimingInstances.length>0,"spottingTimingInstances is empty");
    var meanT=0.0;
    var meanS=0.0;
    var meanE=0.0;
    var allCSV='numTrue,numFalse,numAmbig,skip,error,acc,time,total,prev,user\n';
    var avgN={};
    var countN={};
    var avgTime=0;
    var countTime=0;
    var avgSkipT = [0,0,0,0,0,0];
    var avgErrorT = [0,0,0,0,0,0];
    var counterT = [0,0,0,0,0,0];
    for (var inst of spottingTimingInstances) {
        var accuracy = inst.accuracy;
        meanT += inst.numT/(inst.total+0.0);
        meanS += inst.numSkip/(inst.total+0.0);
        meanE += 1-accuracy;
        allCSV += inst.numT+','+inst.numF+','+inst.numA+','+inst.numSkip+','+( 1-accuracy)+','+accuracy+','+inst.time+','+inst.total+','+(inst.prevSame?1:0)+','+inst.user+'\n';
        if (!avgN.hasOwnProperty(inst.ngram.length)) {
            avgN[inst.ngram.length]=0;
            countN[inst.ngram.length]=0.0;
        }
        avgN[inst.ngram.length]+=inst.time;
        countN[inst.ngram.length]+=(0.0+inst.total);
        avgTime+=inst.time;
        countTime+=(0.0+inst.total);

        avgSkipT[inst.numT] += inst.numSkip;
        avgErrorT[inst.numT] += ( 1-accuracy);
        counterT[inst.numT] += 1;
    }
    meanT /= spottingTimingInstances.length;
    meanS /= spottingTimingInstances.length;
    meanE /= spottingTimingInstances.length;
    console.log('T:'+meanT+' S:'+meanS+' E:'+meanE);
    console.log('numT (per inst),skip (per inst),error');
    for (var t=0; t<=5; t++) {
        console.log(t+','+(avgSkipT[t]/counterT[t])+','+(avgErrorT[t]/counterT[t]));
    }

    console.log('times, per instance');
    console.log('ALL: '+(avgTime/countTime));
    for (var n in avgN) {
        console.log(n+': '+(avgN[n]/countN[n]));
    }

    var stream = fs.createWriteStream("user/spottings_all_"+dataname+".csv");
    stream.once('open', function(fd) {
        stream.write(allCSV);
        stream.close();
    });
    console.log('---done spottings---');
}


//time = c + numT*t + numSkip*s + same*p + acc*a

var Database = require('./database')();
var database=new Database('localhost:27017/cattss', datanames.concat(lineDatanames), function(db){
    console.log('Start stats collecting');
    //store as such{ngram, numSkip, numT, numF, prevSame, numObv, accuracy, time}
    //
    for (var i=0; i<datanames.length; i++)
    {
        console.log('--'+datanames[i]+'--');
        //db.getNewSpottingTimings(datanames[i],findParams2);
        //db.getManTimings(datanames[i],findParamsMan);
        db.getTransTimings(datanames[i],findParamsTransNew);
    }
    //db.getDoubleTransTimings('BENTHAMfancy','BENTHAMcluster_step',findParamsTrans);
    //db.getDoubleTransTimings('NAMESfancy','NAMEScluster_step',findParamsTrans);
    for (var i=0; i<lineDatanames.length; i++)
    {
        console.log('--'+lineDatanames[i]+'--');
        db.getManTimings(lineDatanames[i],findParamsManNew);
    }
});

