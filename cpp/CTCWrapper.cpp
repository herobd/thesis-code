#include "CTCWrapper.h"
/*
CTCWrapper::CTCWrapper(int maxInputLen, int maxLabelLen) : maxInputLen(maxInputLen), maxLabelLen(maxLabelLen)
{
    memset(&options, 0, sizeof(options));
    options.loc = CTC_CPU;
    //options.blank_label=26;
    options.blank_label=0;
    options.num_threads=0;
    alphaSize = 27;
    size_t worksize=0;
    ctcStatus_t stat = get_workspace_size(&maxLabelLen,&maxInputLen,alphaSize,1,options,&worksize);
    if (stat!=CTC_STATUS_SUCCESS)
        cout<<"Warp CTC failed with "<<stat<<endl;
    assert(stat==CTC_STATUS_SUCCESS);
    //cout<<"maxInputLen: "<<maxInputLen<<"  maxLabelLen: "<<maxLabelLen<<endl;
    //cout<<"mallocing "<<worksize<<endl;
    workspace = new char[worksize]; //malloc(worksize);
    probs = new float[alphaSize * maxInputLen];
    grads = new float[alphaSize * maxInputLen];
    labels = new int[maxLabelLen];
    //cout<<"success"<<endl;
    assert(workspace != NULL);
}

float CTCWrapper::loss(Mat cpv, string label)
{
    if (cpv.rows<alphaSize)
    {
        Mat blank = Mat::zeros(1,cpv.cols,CV_32F);
        if (options.blank_label==26)
            vconcat(cpv,blank,cpv);
        else
            vconcat(blank,cpv,cpv);
    }
    if (label.length()>cpv.cols)
    {
        Mat side = Mat::zeros(cpv.rows,label.length()-cpv.cols +1,CV_32F);
        side.row(options.blank_label)=1;
        hconcat(cpv,side,cpv);
    }
    assert(cpv.rows == alphaSize);
    for (int t=0; t<cpv.cols; t++)
        for (int p=0; p<alphaSize; p++)
            probs[p+alphaSize*t] = cpv.at<float>(p,t);
    for (int i=0; i<label.length(); i++)
    {
        if (label[i]<'a' || label[i]>'z')
            return 99999;
        int letterIdx = label[i]-'a'+1;
        //labels[i*alphaSize + letterIdx]=1;
        labels[i]=letterIdx;
    }
    int labelLen = label.length();
    int inputLen =  cpv.cols;
    assert(labelLen<=maxLabelLen && inputLen<=maxInputLen);
    float cost;

    ///
    //for (int ii=0; ii<inputLen; ii++)
    //{
    //    cout<<"in: "<<probs[0+ii*alphaSize]<<", "<<probs[1+ii*alphaSize]<<", "<<probs[2+ii*alphaSize]<<", "<<probs[3+ii*alphaSize]<<", "<<probs[4+ii*alphaSize]<<endl;
    //}
    //for (int ii=0; ii<labelLen; ii++)
    //    cout<<labels[ii]<<endl;
    ///

    ctcStatus_t stat = compute_ctc_loss(probs, grads, labels, &labelLen, &inputLen, alphaSize, 1, &cost,(void*) workspace, options);
    if (stat!=CTC_STATUS_SUCCESS)
        cout<<"Warp CTC failed with "<<stat<<endl;
    assert(stat==CTC_STATUS_SUCCESS);

    return cost/cpv.cols;

    
    //float errorSum=0;
    //cout<<"Error:"<<endl;
    //for (int t=0; t<cpv.cols; t++)
    //{
    //    for (int p=0; p<5; p++)
    //        cout<<grads[p+alphaSize*t]<<", ";
    //    cout<<endl;
    //}
    //for (int i=0; i<cpv.cols*alphaSize; i++)
    //    errorSum += grads[i]*grads[i];
    //return errorSum/cpv.cols;
    

}*/

CTCWrapper::CTCWrapper(int maxInputLen, int maxLabelLen) : maxInputLen(maxInputLen), maxLabelLen(maxLabelLen)
{
    alphaSize = 26;
    probs = new double[alphaSize * maxInputLen];
    labelData = new double[alphaSize * maxInputLen];// maxLabelLen];
}

float CTCWrapper::loss(Mat cpv, string label)
{
    assert(cpv.rows==alphaSize);
    DFeatureVector fvCPV;
    for (int t=0; t<cpv.cols; t++)
        for (int p=0; p<cpv.rows; p++)
            probs[p*cpv.cols+t] = cpv.at<float>(p,t);
    fvCPV.setData_dbl(probs, cpv.cols, cpv.rows,true,false);//,
                               //bool fInputDataGroupedByDimension = true,
                               //                   bool fCopy = true,
                               //                                      bool fStoreDataGroupedByDimension = true);
    DFeatureVector fvLabel;
    //int splicesPerLetter = cpv.cols/label.length();
    memset(labelData,0,sizeof(double)*label.length()*alphaSize);//*splicesPerLetter);
    for (int i=0; i<label.length(); i++)
    {
        int letterIdx = label[i]-'a';
        if (letterIdx<0 || letterIdx>alphaSize)
            return 99999;
        //for (int ii=0; ii<splicesPerLetter; ii++)
        //    labelData[ii+ i*splicesPerLetter + letterIdx*splicesPerLetter*label.length()]=1;
        labelData[i + letterIdx*label.length()]=1;
    }
    //fvLabel.setData_dbl(labelData, label.length()*splicesPerLetter, alphaSize,true,false);
    fvLabel.setData_dbl(labelData, label.length(), alphaSize,true,false);

    int bandRad = 300;
    double bandCost = 0;
    double nonDiagonalCost = 0;//.00000000000001;
    int pathLen;
    int *path = new int[(1+fvCPV.vectLen)*(1+fvLabel.vectLen)];
    float score = DDynamicProgramming::findDPAlignment(fvCPV,fvLabel,bandRad,bandCost,nonDiagonalCost,&pathLen,path);
    
    /*
    double minV,maxV;
    minMaxLoc(cpv,&minV,&maxV);
    int cpvI=-1;
    int labelI=-1;
    Mat draw(26*2 + 1,pathLen+1,CV_8UC3);
    for (int c=0; c<draw.cols; c++)
    {
        draw.at<Vec3b>(26,c)=Vec3b(255,0,0);
        for (int r=27; r<draw.rows; r++)
        {
            draw.at<Vec3b>(r,c)=Vec3b(0,0,0);
        }
    }

    cout<<"lable: "<<label<<"   Path: "<<endl;

    for (int p=0; p<pathLen; p++)
    {
        cout<<path[p]<<", ";
        if (path[p]==0)
        {
            cpvI++;
            labelI++;
        }
        else if (path[p]==2)//rev
        {
            cpvI++;
        }
        else if (path[p]==1)//rev
        {
            labelI++;
        }
        assert(cpvI<cpv.cols);
        //assert(labelI<label.length()*splicesPerLetter);
        assert(labelI<label.length());
        if (cpvI<0)
        {
            for (int r=0; r<26; r++)
                draw.at<Vec3b>(r,p)=Vec3b(0,0,0);
        }
        else
        {
            for (int r=0; r<26; r++)
            {
                float v = cpv.at<float>(r,cpvI);
                int red=255;
                int green=255;
                if (v<0.5)
                    green=255*v/0.5;
                else
                    red=255*(1-v)/0.5;
                red *= (v-minV)/(maxV-minV);
                green *= (v-minV)/(maxV-minV);
                draw.at<Vec3b>(r,p)=Vec3b(0,green,red);
            }
        }
        if (labelI>=0)
        {
            //int letterIdx = label[labelI/splicesPerLetter]-'a';
            int letterIdx = label[labelI]-'a';
            draw.at<Vec3b>(letterIdx+27,p)=Vec3b(0,255,0);
        }
    }
    draw(Rect(pathLen,0,1,26*2+1))=Scalar(0,0,0);
    for (int i=0; i<label.length(); i++)
    {
        int letterIdx = label[i]-'a';
        draw.at<Vec3b>(letterIdx,pathLen)=Vec3b(0,255,0);
    }
    cout<<endl;
    //cout<<"maxV: "<<maxV<<endl;
    imshow("aligned",draw);
    waitKey();
//*/
    delete [] path;
    
    return score;
}
