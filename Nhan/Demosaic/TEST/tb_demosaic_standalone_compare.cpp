#include "isp_demosaic.h"
#include "golden_demosaic_model.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int main(){
    const int W=64,H=48,F=2,N=W*H,DELAY=2*W+2,RTL_MARGIN=16,T=270,M=23;
    std::vector<golden_demosaic_model::RgbFloat> rf_all;
    std::vector<golden_demosaic_model::Rgb12> rq_all;
    for(int f=0;f<F;++f){
        std::vector<std::uint16_t> raw(N);
        for(int r=0;r<H;++r)
            for(int c=0;c<W;++c)
                raw[r*W+c]=(f*509+r*97+c*53+((r*c*11)^(c*29)))&0xfff;
        const auto rf=golden_demosaic_model::demosaic_float(raw,W,H,T,M);
        const auto rq=golden_demosaic_model::demosaic_fixed(raw,W,H,T,M);
        rf_all.insert(rf_all.end(),rf.begin(),rf.end());
        rq_all.insert(rq_all.end(),rq.begin(),rq.end());
    }
    hls::stream<video_in_t> si;hls::stream<video_out_t> so;
    std::uint64_t mismatch=0,samples=0;
    double sae=0,sse=0,maxe=0;
    int oi=0;

    // Khong doc output ngay sau tung lan goi. CSim ket thuc loi goi theo mo
    // hinh C, con RTL ap_ctrl_none co them pipeline latency. Doc theo tung
    // call se lam C/RTL post-check gan sai token output cho chi so golden.
    const int total_inputs=F*N+DELAY+RTL_MARGIN;
    for(int n=0;n<total_inputs;++n){
        int f=n/N,k=n%N,r=k/W,c=k%W;
        video_in_t p;
        p.data=(f*509+r*97+c*53+((r*c*11)^(c*29)))&0xfff;
        p.keep=-1;
        p.strb=-1;
        p.user=(k==0);
        p.last=(c==W-1);
        p.id=0;
        p.dest=0;
        si.write(p);
        isp_demosaicing_top(si,so,W,H,T,M);
    }

    // RTL_MARGIN tao cac transaction bo sung de day het pipeline RTL.
    // Chi F*N output dau la du lieu cua hai frame can kiem tra; output padding
    // sau do duoc bo qua.
    while(!so.empty()){
        const video_out_t q=so.read();
        if(oi>=F*N) continue;
        unsigned d[3]={(unsigned)q.data&0xfff,(unsigned)(q.data>>12)&0xfff,(unsigned)(q.data>>24)&0xfff};
        unsigned ref[3]={rq_all[oi].r,rq_all[oi].g,rq_all[oi].b};
        double fl[3]={rf_all[oi].r,rf_all[oi].g,rf_all[oi].b};
        for(int ch=0;ch<3;++ch){
            if(d[ch]!=ref[ch]){
                if(mismatch<10)std::cerr<<"DEM mismatch i="<<oi<<" ch="<<ch<<" DUT="<<d[ch]<<" FIX="<<ref[ch]<<'\n';
                ++mismatch;
            }
            double e=(double)ref[ch]-fl[ch];
            sae+=std::fabs(e);
            sse+=e*e;
            maxe=std::max(maxe,std::fabs(e));
            ++samples;
        }
        ++oi;
    }
    if(oi!=F*N){
        std::cerr<<"[FAIL] Demosaic output count="<<oi<<" expected="<<F*N<<'\n';
        return 1;
    }
    std::cout<<"Demosaic HLS-fixed mismatches="<<mismatch<<'/'<<samples<<'\n'<<"Demosaic fixed-float MAE="<<sae/samples<<" MSE="<<sse/samples<<" MAX="<<maxe<<'\n';
    return mismatch?1:0;
}
