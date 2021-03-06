//
// Created by Augustin Pan on 4/15/20.
//

#ifndef LIB_OPERATION_H
#define LIB_OPERATION_H
#define PI 3.14159
#define HORIZONTAL "Horizontal"
#define VERTICAL "Vertical"

#include <random>
#include <chrono>
#include <utility>
#include <iostream>
#include "filters.h"
#include <algorithm>
#include <vector>
#include <limits>
#include <type_traits>


namespace augmentorLib {

    const double LOWER_BOUND_PROB = 0.0;
    const double UPPER_BOUND_PROB = 1.0;
    const unsigned NULL_SEED = 0;

    /// A class to generate random numbers
    /// It has different implementations based on the datatype. It uses `uniform_real_distribution`
    /// to generate random numbers of floating numbers, and `uniform_int_distribution` to generate integers.
    ///
    template <typename DataType, bool IsReal = std::is_floating_point<DataType>::value>
    class UniformDistributionGenerator;

    ///
    /// A class to generate floating numbers
    ///
    template <typename DataType>
    class UniformDistributionGenerator<DataType, true> {
    private:
        std::default_random_engine generator;
        std::uniform_real_distribution<DataType> distribution;

        static unsigned init_seed(unsigned seed) {
            if (seed == NULL_SEED) {
                return std::chrono::system_clock::now().time_since_epoch().count();
            }
            return seed;
        }

    public:
        UniformDistributionGenerator(): UniformDistributionGenerator<DataType>(NULL_SEED) {};

        ~UniformDistributionGenerator() = default;

        ///
        /// if seed = 0, the program will automatically generate a seed based on current time.
        /// The range of random numbers is from 0 to 1.
        ///
        explicit UniformDistributionGenerator(unsigned seed):
                generator{init_seed(seed)},
                distribution{LOWER_BOUND_PROB, UPPER_BOUND_PROB} {}

        explicit UniformDistributionGenerator(unsigned seed, DataType lower, DataType upper):
                generator{init_seed(seed)},
                distribution{lower, upper} {}

        inline DataType operator()() {
            return distribution(generator);
        }
    };

    /// A class to generate int numbers
    /// Int numbers include from int8 to unsigned long long.
    ///
    template <typename DataType>
    class UniformDistributionGenerator<DataType, false> {
    private:
        std::default_random_engine generator;
        std::uniform_int_distribution<DataType> distribution;

        static unsigned init_seed(unsigned seed) {
            if (seed == NULL_SEED) {
                return std::chrono::system_clock::now().time_since_epoch().count();
            }
            return seed;
        }

    public:
        UniformDistributionGenerator(): UniformDistributionGenerator<DataType>(NULL_SEED) {};

        ~UniformDistributionGenerator() = default;

        ///
        /// if seed = 0, the program will automatically generate a seed based on current time.
        /// The range of random numbers from Datatype_min to Datatype_max
        ///
        explicit UniformDistributionGenerator(unsigned seed):
                generator{init_seed(seed)},
                distribution{std::numeric_limits<DataType>::min(), std::numeric_limits<DataType>::max()} {}

        ///
        /// if seed = 0, the program will automatically generate a seed based on current time.
        ///
        explicit UniformDistributionGenerator(unsigned seed, DataType lower, DataType upper):
                generator{init_seed(seed)},
                distribution{lower, upper} {}

        inline DataType operator()() {
            return distribution(generator);
        }
    };

    //TODO: use concept to constrain the value type to images
    /// An operation class that is used is used as a Base class to create other operations
    ///
    /// \tparam Image Takes an Image as a template parameter
    template<typename Image>
    class Operation {
    private:
        typedef double _precision_type;
        double probability;
        UniformDistributionGenerator<_precision_type> generator;

    protected:
        /// Used to decide whether an operation is performed or not
        /// \return A boolean value indicating whether the operation must be performed or not based on probability
        inline bool operate_this_time() {
            return generator() <= probability;
        }

        inline _precision_type uniform_random_number() {
            return generator();
        }

        inline _precision_type uniform_random_number(const _precision_type lower, const  _precision_type upper) {
            return (upper - lower) * generator() + lower;
        }

    public:

        /// Default constructor
        Operation(): probability{UPPER_BOUND_PROB}, generator{NULL_SEED} {};

        /// Destructor for the Operation class
        virtual ~Operation() = default;

        /// Parameterized constructor
        ///
        /// \param prob Proabability of performing an operation
        /// \param seed The random seed for the randomness of the operation
        explicit Operation(double prob, unsigned seed = NULL_SEED): probability{prob}, generator{seed} {}

        template <typename Container>
        Container&& perform(Container&&);

        // use pointer here, because we can use nullptr to indicate the Operation did not occur.
        /// Perform function that is called to invoke a particular operation
        ///
        /// \param image Image to perform an operaion on
        /// \return A pointer to an image object
        virtual Image* perform(Image* image) = 0;
    };


    template<typename Image>
    class StdoutOperation: public Operation<Image> {
    private:
        std::string str;
    public:
        StdoutOperation(): str{""}, Operation<Image>{} {};

        explicit StdoutOperation(std::string s, double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED):
                Operation<Image>{prob, seed}, str{std::move(s)} {};

        Image * perform(Image* image) override;

    };

    struct image_size {
        size_t height;
        size_t width;
    };

    template<typename Image>
    class ResizeOperation: public Operation<Image> {
    private:
        image_size lower;
        image_size upper;

    public:
        ResizeOperation() = delete;

        explicit ResizeOperation(image_size lower, image_size upper, double prob = UPPER_BOUND_PROB,
                                 unsigned seed = NULL_SEED): Operation<Image>{prob, seed}, lower{lower}, upper{upper} {};

        Image * perform(Image* image) override;

    };

    template<typename Image>
    class CropOperation: public Operation<Image> {
    private:
        image_size size;
        bool center; //True - use fixed center. False - use random center

    public:
        CropOperation() = delete;

        explicit CropOperation(image_size size, bool center,  double prob = UPPER_BOUND_PROB,
                                 unsigned seed = NULL_SEED): Operation<Image>{prob, seed}, size{size}, center{center} {};

        Image * perform(Image* image) override;

    };

    struct rotate_range {
        int min_rotate;
        int max_rotate;
    };

    template<typename Image>
    class RotateOperation: public Operation<Image> {
    private:
        rotate_range range;

    public:
        RotateOperation() = delete;

        explicit RotateOperation(rotate_range range, double prob = UPPER_BOUND_PROB,
                               unsigned seed = NULL_SEED): Operation<Image>{prob, seed}, range{range} {};

        Image * perform(Image* image) override;

    };

    struct zoom_factor {
        double min_factor;
        double max_factor;
    };

    template<typename Image>
    class ZoomOperation: public Operation<Image> {
    private:
        zoom_factor factor;
//        bool center; //True - use fixed center. False - use random center

    public:
        ZoomOperation() = delete;

        explicit ZoomOperation(zoom_factor factor,  double prob = UPPER_BOUND_PROB,
                               unsigned seed = NULL_SEED): Operation<Image>{prob, seed}, factor{factor} {};

        Image * perform(Image* image) override;

    };


    template<typename Image>
    class InvertOperation: public Operation<Image> {
    public:
        explicit InvertOperation(double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED):
                Operation<Image>{prob, seed} {}

        Image * perform(Image* image) override;

    };

    template<typename Image, int Kernel = 0>
    class GaussianBlurOperation: public Operation<Image> {
    private:
        gaussian_blur_filter_1D<Kernel> filter;
    public:
        explicit GaussianBlurOperation(const double sigma, const size_t n,
                double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED): Operation<Image>{prob, seed},
                filter(sigma, n) {}

        explicit GaussianBlurOperation(const double sigma, double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED):
            Operation<Image>{prob, seed}, filter(sigma) {}

        Image* perform(Image* image) override;

    };


    template<typename Image>
    class BoxBlurOperation: public Operation<Image>{
    private:
        box_blur_filter_1D filter;
    public:
        explicit BoxBlurOperation(const size_t n,
                double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED):
                Operation<Image>{prob, seed}, filter{n} {}

        explicit BoxBlurOperation(const box_blur_filter_1D filter, double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED):
                Operation<Image>{prob, seed}, filter{filter} {}

        Image* perform(Image* image) override;
    };

    template<typename Image>
    class FastGaussianBlurOperation: public Operation<Image> {
    private:
        std::vector<BoxBlurOperation<Image>> box_blur_operations;
    public:

        explicit FastGaussianBlurOperation(const double sigma, const unsigned int passes,
                double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED):
                Operation<Image>{prob, seed} {
            auto filters = box_blur_filter_1D::pseudo_gaussian_filter(sigma, passes);
            for (auto filter : filters) {
                box_blur_operations.push_back(BoxBlurOperation<Image>(filter));
            }
        }

        Image* perform(Image* image) override;

    };

    template<typename Image>
    class RandomEraseOperation: public Operation<Image> {
    private:
        typedef typename Image::pixel_value_type pixel_value_type;
        UniformDistributionGenerator<size_t> xy_generator;
        UniformDistributionGenerator<pixel_value_type> noise_generator;
        image_size lower_mask_size;
        image_size upper_mask_size;
    public:
        explicit RandomEraseOperation(image_size lower_mask_size, image_size upper_mask_size,
                double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED, unsigned xy_seed = NULL_SEED,
                unsigned noise_seed = NULL_SEED):
                Operation<Image>{prob, seed},
                xy_generator(xy_seed),
                noise_generator(noise_seed),
                lower_mask_size{lower_mask_size}, upper_mask_size{upper_mask_size} {}


        Image * perform(Image* image) override;

    };

    template<typename Image>
    class FlipOperation: public Operation<Image> {
    private:
        const std::string& type;
    public:
        explicit FlipOperation(const std::string& type,
                               double prob = UPPER_BOUND_PROB, unsigned seed = NULL_SEED): Operation<Image>{prob, seed},
                                                                                           type(type) {}//super.

        Image * perform(Image* image) override;

    };

    template<typename Image>
    Image *FlipOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }

        if(type==HORIZONTAL)
        {
            for(size_t y = 0; y < image->getHeight(); ++y) {
                for(size_t x = 0; x < image->getWidth()/2; ++x) {
                    std::vector<uint8_t> left_pixels = image->getPixel(x, y);
                    std::vector<uint8_t> right_pixels = image->getPixel(image->getWidth() - x - 1, y);

                    image->setPixel(x, y, right_pixels);
                    image->setPixel(image->getWidth()-x-1, y, left_pixels);
                }
            }
        } else if(type==VERTICAL){
            for(size_t y = 0; y < image->getHeight()/2; ++y) {
                for(size_t x = 0; x < image->getWidth(); ++x) {
                    std::vector<uint8_t> top_pixels = image->getPixel(x, y);
                    std::vector<uint8_t> bottom_pixels = image->getPixel(x, image->getHeight() - y - 1);

                    image->setPixel(x, y, bottom_pixels);
                    image->setPixel(x, image->getHeight() - y - 1, top_pixels);
                }
            }
        }
        else
        {
            throw std::out_of_range("Unknown Flip type - Choose wither 'Horizontal' or 'Vertical'");
        }
        return image;
    }


    // Below is the implementation
    template<typename Image>
    template<typename Container>
    Container&& Operation<Image>::perform(Container&& container) {
        auto results = Container();
        for (auto& image : container) {
            results.push(Operation<Image>::perform(image));
        }
        return results;
    }


    template<typename Image>
    Image * StdoutOperation<Image>::perform(Image* image) {
        if (!Operation<Image>::operate_this_time()) {
            return nullptr;
        }
        //std::cout << "(Image*) Stdout Operation is called:" << std::endl << str << std::endl;
        return image;
    }


    template<typename Image>
    Image *ResizeOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }
        auto factor = Operation<Image>::uniform_random_number();
        int height = (upper.height - lower.height) * factor + lower.height;
        int width = (upper.width - lower.width) * factor + lower.width;

        image->resize(height, width);
        return image;
    }

    template<typename Image>
    Image *CropOperation<Image>::perform(Image *image) {;
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }

        int w = image->getWidth();
        int h = image->getHeight();


        Image temp(size.width, size.height);
        if (center){
            auto x = w/2;
            auto y = h/2;


            auto left_offset = x - size.width/2;
            auto down_offset = y - size.height/2;

//            if (left_offset < 0 || left_offset >= w) {
//                throw std::out_of_range("xOffset is out of range");
//            }
//            if (down_offset < 0 || down_offset >= h){
//                throw std::out_of_range("yOffset is out of range");
//            }
//            if (size.width < 0 || (size.width + left_offset) >= w) {
//                throw std::out_of_range("widthCrop is out of range");
//            }
//            if (size.height < 0 || (size.height + down_offset) >= h) {
//                throw std::out_of_range("heightCrop is out of range");
//            }
//            std::cout<<temp->getWidth()<<" "<<temp->getHeight()<<std::endl;

            for(unsigned long i=left_offset, i1=0; i<left_offset+size.width; i++, i1++){
                for(unsigned long  j=down_offset, j1=0; j<down_offset+size.height; j++, j1++){
                    temp.setPixel(i1, j1, image->getPixel(i,j));
                }
            }

        }
        else{
//            TODO: For random centers
//            auto left_shift = Operation<Image>::uniform_random_number(0, w - size.width);
//            auto down_shift = Operation<Image>::uniform_random_number(0, h - size.height);
        }

        *image = temp;
        return image;
    }

    template<typename Image>
    Image *ZoomOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }

        double zoom_level = Operation<Image>::uniform_random_number(factor.min_factor, factor.max_factor);
        zoom_level = static_cast<float>(static_cast<int>(zoom_level * 10.)) / 10.;

        int w = image->getWidth();
        int h = image->getHeight();

        //TODO: int double issue - very sloow
        int w_zoomed = w*zoom_level;
        int h_zoomed = h*zoom_level;

        image->resize(h_zoomed, w_zoomed);

        auto operation = CropOperation<Image>(
                image_size{static_cast<size_t>(h), static_cast<size_t>(w)}, true, 1
        );

        image = operation.perform(image);

        return image;
    }

    template<typename Image>
    Image *RotateOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }


        double rotate_degree = Operation<Image>::uniform_random_number(range.min_rotate, range.max_rotate);

        int w = image->getWidth();
        int h = image->getHeight();
        Image temp(w, h);

        int hwidth = w / 2;
        int hheight = h / 2;
        double angle = rotate_degree * PI / 180.0;

        for (int x = 0; x < w;x++) {

            for (int y = 0; y < h;y++) {


                int xt = x - hwidth;
                int yt = y - hheight;


                int xs = (int)round((cos(angle) * xt - sin(angle) * yt) + hwidth);
                int ys = (int)round((sin(angle) * xt + cos(angle) * yt) + hheight);


                if (xs >= 0 && xs < w && ys >= 0 && ys < h){
                    temp.setPixel(x, y, image->getPixel(xs, ys));
                }

            }
        }

        *image = temp;
        return image;
    }

    template<typename Image>
    Image *InvertOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }
        // Invert image
        for(size_t y = 0; y < image->getHeight(); y++) {
            for(size_t x = 0; x < image->getWidth(); x++) {
                std::vector<uint8_t> pixels = image->getPixel(x, y);

                for(uint8_t &p: pixels){
                    p = 255-p;
                }
                image->setPixel(x,y,pixels);
            }
        }
        return image;
    }


    inline void convert2pixel(const std::vector<double>& src, std::vector<uint8_t>& target, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            target[i] = src[i];
        }
    }

    template<typename Image, int Kernel>
    Image *GaussianBlurOperation<Image, Kernel>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }
        auto kernel_size = filter.size();
        auto transient = Image(image->getWidth(), image->getHeight(), image->getPixelSize(), image->getColorSpace());
        auto pixel_size = image->getPixelSize();
        auto val = std::vector<double>(pixel_size);
        auto new_pixel = std::vector<uint8_t>(pixel_size);

        // convolute at height axis
        for (size_t i = 0; i< image->getWidth(); ++i) {
            for (size_t j = 0; j < image->getHeight(); ++j) {
                std::fill(val.begin(), val.end(), 0);
                long x0 = i - (kernel_size / 2);

                for (size_t k = 0; k < kernel_size; ++k) {
                    size_t x = std::min((size_t) std::max(x0++, 0l), image->getWidth() - 1);
//                    std::cout << i << " " << kernel_size << " " << x0 << " " <<  x << " " << j << " " << image->getWidth() << std::endl;
                    auto pixel = image->getPixel(x, j);
                    for (size_t p = 0; p < pixel_size; ++p) {
                        val[p] += pixel[p] * filter[k];
                    }
                }
                convert2pixel(val, new_pixel, pixel_size);
                transient.setPixel(i, j, new_pixel);
            }
        }

        // convolute at width axis
        for (size_t i = 0; i< image->getWidth(); ++i) {
            for (size_t j = 0; j < image->getHeight(); ++j) {
                std::fill(val.begin(), val.end(), 0);
                long y0 = j - (kernel_size / 2);

                for (size_t k = 0; k < kernel_size; ++k) {
                    size_t y = std::min((size_t) std::max(y0++, 0l), image->getHeight() - 1);
                    auto pixel = image->getPixel(i, y);
                    for (size_t p = 0; p < pixel_size; ++p) {
                        val[p] += pixel[p] * filter[k];
                    }
                }
                convert2pixel(val, new_pixel, pixel_size);
                image->setPixel(i, j, new_pixel);
            }
        }

        return image;
    }

    struct accumulator {
        typedef uint64_t _datatype;
        std::vector<_datatype> values;

        explicit accumulator(size_t n): values(n) {}

        template <typename Value>
        inline void add(std::vector<Value>&& val) {
            for (size_t i = 0; i < values.size(); ++i) {
                values[i] += val[i];
            }

        }

        template <typename Value>
        inline void shift(std::vector<Value>&& del, std::vector<Value>&& add) {
            for (size_t i = 0; i < values.size(); ++i) {
                values[i] += add[i];
                values[i] -= del[i];
            }

        }

        template <typename ReturnType=u_int8_t >
        inline std::vector<ReturnType> div(_datatype denominator) {
            std::vector<ReturnType> res(values.size());
            for (size_t i = 0; i < values.size(); ++i) {
                res[i] = values[i] / denominator;
            }
            return res;
        }
    };

    template<typename Image>
    Image *BoxBlurOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }

        auto transient = Image(image->getWidth(), image->getHeight(), image->getPixelSize(), image->getColorSpace());
        auto pixel_size = image->getPixelSize();

        for (size_t i = 0; i< image->getWidth(); ++i) {
            auto acc = accumulator(pixel_size);

            long y0 = -(filter.length / 2);
            for (size_t k = 0; k < filter.length; ++k) {
                size_t y = std::min((size_t) std::max(y0++, 0l), image->getHeight() - 1);
                acc.add(image->getPixel(i, y));
                transient.setPixel(i, 0, acc.div(filter.length));
            }

            long y_del = -(filter.length / 2);
            size_t y_add = (filter.length / 2) + 1;
            for (size_t j = 1; j < image->getHeight(); ++j) {
                size_t prev = std::max(y_del++, 0l);
                size_t next = std::min(y_add++, image->getHeight() - 1);
                acc.shift(image->getPixel(i, prev), image->getPixel(i, next));
                transient.setPixel(i, j, acc.div(filter.length));
            }
        }

        for (size_t j = 0; j< image->getHeight(); ++j) {
            auto acc = accumulator(pixel_size);

            long x0 = -(filter.length / 2);
            for (size_t k = 0; k < filter.length; ++k) {
                size_t x = std::min((size_t) std::max(x0++, 0l), image->getWidth() - 1);
                acc.add(transient.getPixel(x, j));
                image->setPixel(0, j, acc.div(filter.length));
            }

            long x_del = -(filter.length / 2);
            size_t x_add = (filter.length / 2) + 1;
            for (size_t i = 1; i < image->getWidth(); ++i) {
                size_t prev = std::max(x_del++, 0l);
                size_t next = std::min(x_add++, image->getWidth() - 1);
                acc.shift(transient.getPixel(prev, j), transient.getPixel(next, j));
                image->setPixel(i, j, acc.div(filter.length));

            }
        }

        return image;
    }

    template<typename Image>
    Image* FastGaussianBlurOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }
        for (auto operation : box_blur_operations) {
            image = operation.perform(image);
        }
        return image;
    }

    template<typename Image>
    Image* RandomEraseOperation<Image>::perform(Image *image) {
        if (!Operation<Image>::operate_this_time()) {
            return image;
        }

        auto lower_erase_size = image_size{
                std::min(image->getHeight(), lower_mask_size.height),
                std::min(image->getWidth(), lower_mask_size.width),
        };

        auto upper_erase_size = image_size{
                std::min(image->getHeight(), upper_mask_size.height),
                std::min(image->getWidth(), upper_mask_size.width),
        };

        auto factor = RandomEraseOperation<Image>::uniform_random_number();
        auto erase_size = image_size{
                (size_t) ((upper_erase_size.height - lower_erase_size.height) * factor) + lower_erase_size.height,
                (size_t) ((upper_erase_size.width - lower_erase_size.width) * factor) + lower_erase_size.width
        };

        auto top = xy_generator() % (image->getHeight() - erase_size.height + 1);
        auto left = xy_generator() % (image->getWidth() - erase_size.width + 1);

        auto pixel_size = image->getPixelSize();
        auto new_pixel = std::vector<RandomEraseOperation::pixel_value_type>(pixel_size);

        for (size_t i = left; i < left + erase_size.width; ++i) {
            for (size_t j = top; j < top + erase_size.height; ++j) {
                for (size_t k = 0; k < pixel_size; ++k) {
                    new_pixel[k] = noise_generator();
                }
                image->setPixel(i, j, new_pixel);
            }
        }

        return image;
    }
}

#endif //LIB_OPERATION_H


