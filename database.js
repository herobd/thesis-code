module.exports =  function() {
   //security experts, please feel free to cringe
   var fjfjfj=[32,523,435,21,2436,36,234,5243,346,34,542,5,24,5243,6256,456,24,65,464,
   65,465,56,56,546,542,6,565,46,654,6,5645,654,64,87,8790,3,571,7548,760,36,87356,76,
   57235,9838,625,645,76587,65338,56387,65,47,658,65,53,5,3,52,5,35,436,76,98,5,975,6,
   34,435,1,0,7,65,4,80,644,5,7,9,6,44,6,8,7,544,5,7,88,456,6,54,5,77,45624,456,6,56,
   34,435,1,0,7,65,4,830,644,5,7,9,6,44,6,8,5,544,5,7,88,456,6,54,5,77,45624,456,6,5];
   var fs = require('fs');
    function Database(address,dataNames,callback) {
        
        var self=this;
        
        self.mongo = require('mongodb').MongoClient;

        // Connect to the db (localhost:27017/exampleDb)
        self.mongo.connect("mongodb://"+address, function(err, db) {
          if(!err) {
            console.log("We are connected to the database.");
            var numCol=1+5*dataNames.length;

            //Connect to all the collections
            db.collection('THESIS_USERS', function(err, collection) {
                if(!err) {
                    self.userCollection=collection;
                    if (--numCol <= 0)
                        callback(self);
                } else {
                    console.log('ERROR: conencting to MongoDB colection THESIS_USERS: '+err);
                }
            });
            
            db.collection('THESIS_ALPHA', function(err, collection) {
                if(!err) {
                    self.alphaCollection=collection;
                    //if (--numCol <= 0)
                    //    callback(self);
                } else {
                    console.log('ERROR: conencting to MongoDB colection THESIS_ALPHA: '+err);
                }
            });
            
            self.savedSpottingsCollection={};
            self.savedTransCollection={};
            self.timingSpottingsCollection={};
            self.timingTransCollection={};
            self.timingManualCollection={};
            dataNames.forEach( function(dataName) {
                var savedSpottings='SAVED_SPOTTINGS_'+dataName;
                var savedTrans = 'SAVED_TRANS_'+dataName;
                var timingSpottings = 'TIMING_SPOTTINGS_'+dataName;
                var timingManual = 'TIMING_MANUAL_'+dataName;
                var timingTrans = 'TIMING_TRANS_'+dataName;
                db.collection(savedSpottings, function(err, collection) {
                    if(!err) {
                        self.savedSpottingsCollection[dataName]=collection;
                        //self.savedSpottingsCollection=collection;
                        if (--numCol <= 0)
                            callback(self);
                    } else {
                        console.log('ERROR: conencting to MongoDB colection SAVED_SPOTTINGS: '+err);
                    }
                });
                db.collection(savedTrans , function(err, collection) {
                    if(!err) {
                        self.savedTransCollection[dataName]=collection;
                        //self.savedTransCollection=collection;
                        if (--numCol <= 0)
                            callback(self);
                    } else {
                        console.log('ERROR: conencting to MongoDB colection SAVED_TRANS: '+err);
                    }
                });
                db.collection(timingSpottings , function(err, collection) {
                    if(!err) {
                        self.timingSpottingsCollection[dataName]=collection;
                        //self.timingSpottingsCollection=collection;
                        if (--numCol <= 0)
                            callback(self);
                    } else {
                        console.log('ERROR: conencting to MongoDB colection TIMING_SPOTTINGS: '+err);
                    }
                });
                db.collection(timingTrans , function(err, collection) {
                    if(!err) {
                        self.timingTransCollection[dataName]=collection;
                        //self.timingTransCollection=collection;
                        if (--numCol <= 0)
                            callback(self);
                    } else {
                        console.log('ERROR: conencting to MongoDB colection TIMING_TRANS: '+err);
                    }
                });
                db.collection(timingManual , function(err, collection) {
                    if(!err) {
                        self.timingManualCollection[dataName]=collection;
                        //self.timingManualCollection=collection;
                        if (--numCol <= 0)
                            callback(self);
                    } else {
                        console.log('ERROR: conencting to MongoDB colection TIMING_MANUAL_'+dataName+': '+err);
                    }
                });
            });
            
          } else {
            console.log('ERROR: conencting to MongoDB: '+err);
          }
        });
        
        //test
        var teststr = 'dsafj435jkhrsa@dssfa-d_asf.asdf';
        var enc = self.hideEmail(teststr);
        var back = self.revealEmail(enc);
        if (teststr!=back)
            console.log('Email scramble failed');

        
    }
    
    Database.prototype.addUser = function (email,survey,callback)  {
        
        var self=this;
        //var hemail = this.hideEmail(email);
        self.findUser(email, function(err,item) {
            if (err) {
                callback(err);
            } else if (item!=null){
                callback("User already exists");
            } else {
                self.userCollection.insert({id:email, presurvey:survey}, {w:1}, function(err, result) {
                    callback(err);
                });
            }
        });
        
        
        
    };

    Database.prototype.updateUser = function (userInfo,callback)  {
        this.userCollection.update({id:userInfo.id},userInfo, {upsert:1, w:1}, callback);
    }

    
    
    Database.prototype.findUser = function (email,callback) {
        var self=this;
        //var hemail = this.hideEmail(email);
        self.userCollection.findOne({id:email}, function(err, item) {
            callback(err,item);
        });
    }
    
    Database.prototype.hideEmail = function(email) {
        var ret='';
        for (var i in email) {
            ret+=String.fromCharCode((email.charCodeAt(i)+fjfjfj[i])%256);
        }
        return ret;
    }
    
        
    Database.prototype.revealEmail = function(enc) {
        var ret=''; 
        //console.log(enc)
        for (var i in enc) {
            
            var v = enc.charCodeAt(i)-fjfjfj[i];
            while (v<0)
                v+=256;
            ret+=String.fromCharCode(v%256);
        }
        return ret;
    }
    
    /*Database.prototype.saveAlphaTest = function(userNum,info,callback) {
        var self=this;
        self.alphaCollection.findOne({_id:(userNum+'')}, function(err, item) {
            if (err) {
                callback(err);
            } else if (item==null) {
                //console.log('adding new user '+userNum);
                self.alphaCollection.insert({_id:(userNum+''), tests:[info]}, {w:1}, callback);
            } else {
                //console.log('updating user '+userNum);
                self.alphaCollection.update({_id:(userNum+'')},{ $push: { tests: info } },{w:1}, callback);
            }
        });
        //self.alphaCollection.insert({_id:(userNum+''), tests:[]}, {w:1}, function(err) {
        //    self.alphaCollection.update({_id:(userNum+'')},{ $push: { tests: info } },{w:1}, callback)
        //});
    }
    
    Database.prototype.saveAlphaSurvey = function(userNum,results,callback) {
        results.email = this.hideEmail(results.email);
        this.alphaCollection.update({_id:(userNum+'')},{$set: { survey:results } } ,{w:1}, callback)
    }
    
    
    Database.prototype.listCountPrefAlpha = function(callback) {
        var ret={app_tap:0, app_hardcore:0};
        var self=this;
        var cursor = self.alphaCollection.find({},{survey:1});
        cursor.each(function(err, doc) {
            if (err) {
                callback(err,ret);
            } else if (doc != null) {
                //console.log(doc)
                if (doc.survey.preference==0)
                    ret.app_tap++;
                else
                    ret.app_hardcore++;
            } else {
                callback(err,ret);
            }
        });
    }*/
    
    Database.prototype.listCSVDumpAlpha = function(callback) {
        var ret='id,preference,device,width,height,first,tap_fp,tap_fn,tap_undos,tap_time,swipe_fp,swipe_fn,swipe_undos,swipe_time,response\n';
        var self=this;
        var cursor = self.alphaCollection.find({});
        cursor.each(function(err, doc) {
            if (err) {
                callback(err,ret);
            } else if (doc != null) {
                //console.log(doc)
                if (doc.tests.length>1) {
                    ret+=doc._id+','
                    if (doc.survey)
                        ret+=doc.survey.preference+','+doc.survey.device+','+doc.survey.screenwidth+','+doc.survey.screeneheight+',';
                    else
                        ret+=' , , , ,';
                    var first = doc.tests[0].version;
                    var tap_index=0;
                    var swipe_index=1;
                    if (first=='app_hardcore') {
                        tap_index=1;
                        swipe_index=0;
                    }
                    ret+=first+','+doc.tests[tap_index].fp+','+doc.tests[tap_index].fn+','+doc.tests[tap_index].undos+','+doc.tests[tap_index].time+','+doc.tests[swipe_index].fp+','+doc.tests[swipe_index].fn+','+doc.tests[swipe_index].undos+','+doc.tests[swipe_index].time;
                    if (doc.survey) {
                        var resp = doc.survey.response.replace(new RegExp('[,"]', 'g'), '*');
                        ret+=',"'+resp+'"\n';
                    }
                }
            } else {
                callback(err,ret);
            }
        });
    };
    Database.prototype.listEmailsAlpha = function(callback) {
        var ret=[];
        var self=this;
        var cursor = self.alphaCollection.find({},{survey:1});
        cursor.each(function(err, doc) {
            if (err) {
                callback(err,ret);
            } else if (doc != null) {
                //console.log(doc)
                if (doc.survey)
                    ret.push(self.revealEmail(doc.survey.email))
            } else {
                callback(err,ret);
            }
        });
    }

    Database.prototype.getSpottingTimings = function(dataname,callback) {
        var ret=[];
        var self=this;
        var cursor = self.timingSpottingsCollection[dataname].find({});
        cursor.each(function(err, doc) {
            if (err) {
                callback(err,ret);
                return;
            } else if (doc!=null) {
                //console.log(doc);
                if (doc.userId != 'herobd@gmail.com' && (doc.userId.length<9 || doc.userId.substr(doc.userId.length-9)!='@test.com')) {
                    ret.push(   {
                                    ngram:doc.ngram,
                                    numSkip:5*(1-doc.didRatio),
                                    numT:5*(doc.trueRatioFull),
                                    numF:5*(1-doc.trueRatioFull),
                                    prevSame:doc.prevNgramSame,
                                    //numObv:
                                    accuracy:doc.accuracy,
                                    time:doc.batchTime
                                });
                }
            } else {
                callback(err,ret);
            }
        });
    }
    /*
    Database.prototype.listEmailsAlpha = function(callback) {
        var ret='';
        var self=this;
        var cursor = self.alphaCollection.find({});
        cursor.each(function(err, doc) {
            if (err) {
                callback(err,ret);
            } else if (doc != null) {
                //console.log(doc)
                if (doc.tests.length>1) {
                    ret+=doc._id+','
                    if (doc.survey && doc.survey.email.length>0)
                        ret+=self.revealEmail(doc.survey.email)+', ';
                }
            } else {
                callback(err,ret);
            }
        });
    };
    */
    
    Database.prototype.saveSpotting = function(dataName,id,spotting) {
        this.savedSpottingsCollection[dataName].update({_id:id},{$set: spotting},{ upsert: true } );
        //this.savedSpottingsCollection.update({_id:id},{$set: spotting},{ upsert: true } );
    };

    Database.prototype.saveTrans = function(dataName,id,trans) {
        this.savedTransCollection[dataName].update({_id:id},{$set: trans},{ upsert: true } );
        //this.savedTransCollection.update({_id:id},{$set: trans},{ upsert: true } );
    };

    Database.prototype.getLabeledSpottings = function(dataName,callback) {
        var cursor=this.savedSpottingsCollection[dataName].find();
        //var cursor=this.savedSpottingsCollection.find();
        var ret=[];
        cursor.each(function(err, doc) {
            if (err) {
                callback(err);
            } else if (doc!=null) {
                ret.push(doc);
            } else {
                callback(null,ret);
            }
        });
        
    };
    Database.prototype.getLabeledTrans = function(dataName,callback) {
        var cursor=this.savedTransCollection[dataName].find();
        //var cursor=this.savedTransCollection.find();
        var ret=[];
        cursor.each(function(err, doc) {
            if (err) {
                callback(err);
            } else if (doc!=null) {
                ret.push(doc);
            } else {
                callback(null,ret);
            }
        });
        
    };

    
    Database.prototype.saveTimingTestSpotting = function(dataName,info,callback) {
        var self=this;
                    //self.timingSpottingsCollection[dataName].insert(info, {w:1}, callback);
        //self.timingSpottingsCollection[dataName].update({userId:info.userId, batchNum:info.batchNum},{$set:info},{upsert:1, w:1}, callback);
        self.timingSpottingsCollection[dataName].findOne({userId:info.userId, batchNum:info.batchNum}, function(err, item) {
        //self.timingSpottingsCollection.findOne({userId:info.userId, batchNum:info.batchNum}, function(err, item) {
            if (err) {
                callback(err);
            } else if (item==null) {
                self.timingSpottingsCollection[dataName].insert(info, {w:1}, callback);
                //self.timingSpottingsCollection.insert(info, {w:1}, callback);
            } else {
                self.timingSpottingsCollection[dataName].update({userId:info.userId, batchNum:info.batchNum},{$set:info},{w:1}, callback);
                //self.timingSpottingsCollection.update({userId:info.userId, batchNum:info.batchNum},{$set:info},{w:1}, callback);
            }
        });
    }
    Database.prototype.saveTimingTestTrans = function(dataName,info,callback) {
        var self=this;
        //self.timingTransCollection[dataName].update({userId:info.userId, batchId:info.batchId},{$set:info},{upsert:1, multi:false, w:1}, callback);
        //console.log('saveTimingTrans '+dataName);
        //console.log(self.timingTransCollection);
        self.timingTransCollection[dataName].findOne({userId:info.userId, batchNum:info.batchNum}, function(err, item) {
        //self.timingTransCollection.findOne({userId:info.userId, batchNum:info.batchNum}, function(err, item) {
            if (err) {
                callback(err);
            } else if (item==null) {
                self.timingTransCollection[dataName].insert(info, {w:1}, callback);
                //self.timingTransCollection.insert(info, {w:1}, callback);
            } else {
                self.timingTransCollection[dataName].update({userId:info.userId, batchNum:info.batchNum},{$set:info},{w:1}, callback);
                //self.timingTransCollection.update({userId:info.userId, batchNum:info.batchNum},{$set:info},{w:1}, callback);
            }
        });
    }
    Database.prototype.saveTimingTestManual = function(dataName,info,callback) {
        var self=this;
        //self.timingManualCollection[dataName].update({userId:info.userId, batchNum:info.batchNum},{$set:info},{upsert:1, w:1}, callback);
        self.timingManualCollection[dataName].findOne({userId:info.userId, batchNum:info.batchNum}, function(err, item) {
        //self.timingManualCollection.findOne({userId:info.userId, batchNum:info.batchNum}, function(err, item) {
            if (err) {
                callback(err);
            } else if (item==null) {
                self.timingManualCollection[dataName].insert(info, {w:1}, callback);
                //self.timingManualCollection.insert(info, {w:1}, callback);
            } else {
                self.timingManualCollection[dataName].update({userId:info.userId, batchNum:info.batchNum},{$set:info},{w:1}, callback);
                //self.timingManualCollection.update({userId:info.userId, batchNum:info.batchNum},{$set:info},{w:1}, callback);
            }
        });
    }

    return Database
};
