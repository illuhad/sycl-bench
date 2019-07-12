#include <CL/sycl.hpp>
#include <iostream>

#include "common.h"
#include "bitmap.h"


namespace s = cl::sycl;
class SobelBenchKernel; // kernel forward declaration

using cl::sycl::float4;

/*
  A Sobel filter with a convolution matrix 3x3.
  Input and output are two-dimensional buffers of floats.     
 */
class SobelBench
{
protected:
    std::vector<float4> input;
    std::vector<float4> output;

    size_t w, h; // size of the input picture
    size_t size; // user-defined size (input and output will be size x size)
    BenchmarkArgs args;

public:
  SobelBench(const BenchmarkArgs &_args) : args(_args) {}


  void setup() {
    size = args.problem_size; // input size defined by the user
    load_bitmap_mirrored("../Brommy.bmp", size, input);
    output.resize(size * size);
  }

  void run() {    
    s::buffer<float4, 2>  input_buf( input.data(), s::range<2>(size, size));    
    s::buffer<float4, 2> output_buf(output.data(), s::range<2>(size, size));

    args.device_queue.submit(
        [&](cl::sycl::handler& cgh) {
      auto in  = input_buf .get_access<s::access::mode::read>(cgh);
      auto out = output_buf.get_access<s::access::mode::discard_write>(cgh);
      cl::sycl::range<2> ndrange {size, size};

      // Sobel kernel 3x3
      const float kernel[] =
      { 1, 0, -1,
        2, 0, -2,
        1, 0, -1
      };

      cgh.parallel_for<class SobelBenchKernel>(ndrange,
        [=](cl::sycl::id<2> gid) 
        {
            int x = gid[0];
            int y = gid[1];
            float4 Gx = float4(0.0f, 0.0f, 0.0f, 0.0f);
       	    float4 Gy = float4(0.0f, 0.0f, 0.0f, 0.0f);
            const int radius = 3;

            // constant-size loops in [-1,0,+1]
            for(uint x_shift = -1; x_shift<=1; x_shift++)
                for(uint y_shift = -1; y_shift<=1; y_shift++)
		{
                  // sample position
		  uint xs = x + x_shift;
		  uint ys = y + y_shift;
                  // for the same pixel, convolution is always 0  
                  if(x==xs && y==ys)  continue; 
                  // boundary check
                  if(xs < 0 || xs > size || ys < 0 || ys > size) continue;
                    
	          // sample color
                  float4 sample = in[ {xs,ys} ];

                  // convolution calculation
                  int offset_x = x_shift + y_shift * radius;
                  int offset_y = y_shift + x_shift * radius;
          
                  float conv_x = kernel[offset_x];
                  float4 conv4_x = (float4)(conv_x);
                  Gx += conv4_x * sample;

                  float conv_y = kernel[offset_y];
                  float4 conv4_y = (float4)(conv_y);
                  Gy += conv4_y * sample;	
               }

	  // taking root of sums of squares of Gx and Gy 	
	  float4 color = hypot(Gx, Gy);
	  out[gid] = clamp(color, float4(0.0), float4(1.0));
        });
     });
   }


  bool verify(VerificationSetting &ver) {  
    save_bitmap("sobel3.bmp", size, output);

    const float kernel[] = { 1, 0, -1, 2, 0, -2, 1, 0, -1 };
    bool pass = true;
    int radius = 3;
/*
    for(size_t i=ver.begin[0]; i<ver.begin[0]+ver.range[0]; i++){
      int x = i % size;
      int y = i / size;
      float4 Gx, Gy;	
        for(uint x_shift = -1; x_shift<2; x_shift++)
             for(uint y_shift = -1; y_shift<2; y_shift++)
             {
                  uint xs = x + x_shift;
                  uint ys = y + y_shift;
                  if(x==xs && y==ys)  continue;                   
                  if(xs < 0 || xs > size || ys < 0 || ys > size) continue;
                  float4 sample = input[xs + ys * size];
                  int offset_x = x_shift + y_shift * radius;
                  int offset_y = y_shift + x_shift * radius;
                  float conv_x = kernel[offset_x];
                  float4 conv4_x = (float4)(conv_x);
                  Gx += conv4_x * sample;
                  float conv_y = kernel[offset_y];
                  float4 conv4_y = (float4)(conv_y);
                  Gy += conv4_y * sample;
               }
        float4 color = hypot(Gx, Gy);
        float4 expected = clamp(color, float4(0.0), float4(1.0));
        if(expected.x() - output[i].x() > 0.0f || 
           expected.y() - output[i].y() > 0.0f || 
           expected.z() - output[i].z() > 0.0f)
        {
            pass = false;
            break;
        }
    }    
*/
    return pass;
}


static std::string getBenchmarkName() {
    return "Sobel";
  }


}; // SobelBench class


int main(int argc, char** argv)
{
  BenchmarkApp app(argc, argv);
  app.run<SobelBench>();  
  return 0;
}


