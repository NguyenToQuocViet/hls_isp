#include "isp_wb.h"
#include "golden_wb_model.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    const int W=64, H=48, F=2, N=W*H;
    const golden_wb_model::FloatGains gf = {
        1.125, 0.9375, 1.0625, 1.25
    };

    const golden_wb_model::FixedGainsQ12 gq = {
        golden_wb_model::quantize_gain_q4_12(gf.r),
        golden_wb_model::quantize_gain_q4_12(gf.gr),
        golden_wb_model::quantize_gain_q4_12(gf.gb),
        golden_wb_model::quantize_gain_q4_12(gf.b)
    };

    // Nạp trực tiếp bit Q4.12 để tránh ap_ufixed tự truncate
    gain_t gr, ggr, ggb, gb;
    gr.range(15, 0)  = gq.r;
    ggr.range(15, 0) = gq.gr;
    ggb.range(15, 0) = gq.gb;
    gb.range(15, 0)  = gq.b;
    hls::stream<video_in_t> si; hls::stream<video_out_t> so;
    std::uint64_t mismatch=0, samples=0; double sae=0, sse=0, maxe=0;
    for(int f=0;f<F;++f){
        std::vector<std::uint16_t> raw(N);
        for(int r=0;r<H;++r)for(int c=0;c<W;++c)
            raw[r*W+c]=(f*113+r*29+c*17+((r*c*3)^(c*11)))&0x3ff;
        const std::vector<double> rf=golden_wb_model::wb_float(raw,W,H,gf);
        const std::vector<std::uint16_t> rq=golden_wb_model::wb_fixed(raw,W,H,gq);
        for(int i=0;i<N;++i){
            video_in_t p; 
            p.data=raw[i]; 
            p.keep=-1; 
            p.strb=-1;
            p.user=(i==0); 
            p.last=(i%W==W-1); 
            p.id=0; 
            p.dest=0; 
            si.write(p);
            isp_wb_top(si,so,gr,ggr,ggb,gb);
            if(so.empty()){
                std::cerr<<"[FAIL] missing WB output\n";
                return 1;
            }
            const unsigned dut=(unsigned)so.read().data;
            if(dut!=rq[i]){
                if(mismatch<10)std::cerr<<"WB mismatch i="<<i<<" DUT="<<dut<<" FIX="<<rq[i]<<'\n';
                ++mismatch;
            }
            const double e=(double)rq[i]-rf[i]; 
            sae+=std::fabs(e); 
            sse+=e*e;
            maxe=std::max(maxe,std::fabs(e)); 
            ++samples;
        }
    }
    std::cout<<"WB HLS-fixed mismatches="<<mismatch<<'/'<<samples<<'\n'
             <<"WB fixed-float MAE="<<sae/samples<<" MSE="<<sse/samples
             <<" MAX="<<maxe<<'\n';
    return mismatch?1:0;
}
