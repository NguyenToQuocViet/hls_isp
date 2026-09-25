#include "isp_ccm.h"
#include "golden_ccm_model.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int main(){
    const int W=64,H=48,F=2,N=W*H;
    const double m[3][3]={{1.125,-.0625,-.0625},{-.03125,1.0625,-.03125},{-.0625,-.0625,1.125}};
    golden_ccm_model::FloatMatrix mf={}; 
    golden_ccm_model::FixedMatrixQ12 mq={};
    for(int r=0;r<3;++r)
        for(int c=0;c<3;++c){
            mf.a[r][c]=m[r][c];
            mq.a[r][c]=golden_ccm_model::quantize_coeff_q4_12(m[r][c]);
        }
    gain_matrix a00=m[0][0],a01=m[0][1],a02=m[0][2],a10=m[1][0],a11=m[1][1],a12=m[1][2],a20=m[2][0],a21=m[2][1],a22=m[2][2];
    hls::stream<video_in_t> si; 
    hls::stream<video_out_t> so;
    std::uint64_t mismatch=0,samples=0; 
    double sae=0,sse=0,maxe=0;
    for(int f=0;f<F;++f){
        std::vector<golden_ccm_model::Rgb12> iq(N);
        std::vector<golden_ccm_model::RgbFloat> inf(N);
        for(int r=0;r<H;++r)
            for(int c=0;c<W;++c){
                int i=r*W+c;iq[i]={(std::uint16_t)((f*211+r*41+c*17)&0xfff),(std::uint16_t)((f*73+r*19+c*37+511)&0xfff),(std::uint16_t)((f*151+r*31+c*13+997)&0xfff)};
                inf[i]={(double)iq[i].r,(double)iq[i].g,(double)iq[i].b};
            }
        const auto rf=golden_ccm_model::ccm_float(inf,mf); 
        const auto rq=golden_ccm_model::ccm_fixed(iq,mq);
        for(int i=0;i<N;++i){
            video_in_t p;p.data=(ap_uint<36>)iq[i].r|((ap_uint<36>)iq[i].g<<12)|((ap_uint<36>)iq[i].b<<24);
            p.keep=-1;
            p.strb=-1;
            p.user=(i==0);
            p.last=(i%W==W-1);
            p.id=0;
            p.dest=0;
            si.write(p);
            isp_ccm_top(si,so,a00,a01,a02,a10,a11,a12,a20,a21,a22);
            if(so.empty()){
                std::cerr<<"[FAIL] missing CCM output\n";
                return 1;
            }
            const video_out_t q=so.read();
            unsigned d[3]={(unsigned)q.data&0xfff,(unsigned)(q.data>>12)&0xfff,(unsigned)(q.data>>24)&0xfff};
            unsigned ref[3]={rq[i].r,rq[i].g,rq[i].b};
            double fl[3]={rf[i].r,rf[i].g,rf[i].b};
            for(int k=0;k<3;++k){
                if(d[k]!=ref[k]){
                    if(mismatch<10)std::cerr<<"CCM mismatch i="<<i<<" ch="<<k<<" DUT="<<d[k]<<" FIX="<<ref[k]<<'\n';
                    ++mismatch;
                }
                double e=(double)ref[k]-fl[k];
                sae+=std::fabs(e);
                sse+=e*e;
                maxe=std::max(maxe,std::fabs(e));
                ++samples;
            }
        }
    }
    std::cout<<"CCM HLS-fixed mismatches="<<mismatch<<'/'<<samples<<'\n'<<"CCM fixed-float MAE="<<sae/samples<<" MSE="<<sse/samples<<" MAX="<<maxe<<'\n';return mismatch?1:0;
}
