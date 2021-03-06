#include "rtkMacro.h"
#include "rtkTest.h"
#include "itkRandomImageSource.h"
#include "rtkConstantImageSource.h"
#include "rtkCudaRayCastBackProjectionImageFilter.h"
#include "rtkCudaForwardProjectionImageFilter.h"

/**
 * \file rtkcudaraycastadjointoperatorstest.cxx
 *
 * \brief Tests whether CUDA ray cast forward and back projectors are matched
 *
 * This test generates a random volume "v" and a random set of projections "p",
 * and compares the scalar products <Rv , p> and <v, R* p>, where R is the 
 * CUDA ray cast forward projector and R* is the CUDA ray cast back projector. If R* is indeed
 * the adjoint of R, these scalar products are equal.
 *
 * \author Cyril Mory
 */

int main(int, char** )
{
  const unsigned int Dimension = 3;
  typedef float                                    OutputPixelType;

#ifdef USE_CUDA
  typedef itk::CudaImage< OutputPixelType, Dimension > OutputImageType;
#else
  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;
#endif

#if FAST_TESTS_NO_CHECKS
  const unsigned int NumberOfProjectionImages = 3;
#else
  const unsigned int NumberOfProjectionImages = 180;
#endif


  // Random image sources
  typedef itk::RandomImageSource< OutputImageType > RandomImageSourceType;
  RandomImageSourceType::Pointer randomVolumeSource  = RandomImageSourceType::New();
  RandomImageSourceType::Pointer randomProjectionsSource = RandomImageSourceType::New();

  // Constant sources
  typedef rtk::ConstantImageSource< OutputImageType > ConstantImageSourceType;
  ConstantImageSourceType::Pointer constantVolumeSource = ConstantImageSourceType::New();
  ConstantImageSourceType::Pointer constantProjectionsSource = ConstantImageSourceType::New();
  
  // Image meta data
  RandomImageSourceType::PointType origin;
  RandomImageSourceType::SizeType size;
  RandomImageSourceType::SpacingType spacing;


  // Volume metadata
  origin[0] = -127.;
  origin[1] = -127.;
  origin[2] = -127.;
#if FAST_TESTS_NO_CHECKS
  size[0] = 2;
  size[1] = 2;
  size[2] = 2;
  spacing[0] = 252.;
  spacing[1] = 252.;
  spacing[2] = 252.;
#else
  size[0] = 64;
  size[1] = 64;
  size[2] = 64;
  spacing[0] = 4.;
  spacing[1] = 4.;
  spacing[2] = 4.;
#endif
  randomVolumeSource->SetOrigin( origin );
  randomVolumeSource->SetSpacing( spacing );
  randomVolumeSource->SetSize( size );
  randomVolumeSource->SetMin( 0. );
  randomVolumeSource->SetMax( 1. );

  constantVolumeSource->SetOrigin( origin );
  constantVolumeSource->SetSpacing( spacing );
  constantVolumeSource->SetSize( size );
  constantVolumeSource->SetConstant( 0. );

  // Projections metadata
  origin[0] = -255.;
  origin[1] = -255.;
  origin[2] = -255.;
#if FAST_TESTS_NO_CHECKS
  size[0] = 2;
  size[1] = 2;
  size[2] = NumberOfProjectionImages;
  spacing[0] = 504.;
  spacing[1] = 504.;
  spacing[2] = 504.;
#else
  size[0] = 64;
  size[1] = 64;
  size[2] = NumberOfProjectionImages;
  spacing[0] = 8.;
  spacing[1] = 8.;
  spacing[2] = 8.;
#endif
  randomProjectionsSource->SetOrigin( origin );
  randomProjectionsSource->SetSpacing( spacing );
  randomProjectionsSource->SetSize( size );
  randomProjectionsSource->SetMin( 0. );
  randomProjectionsSource->SetMax( 100. );

  constantProjectionsSource->SetOrigin( origin );
  constantProjectionsSource->SetSpacing( spacing );
  constantProjectionsSource->SetSize( size );
  constantProjectionsSource->SetConstant( 0. );

  // Update all sources
  TRY_AND_EXIT_ON_ITK_EXCEPTION( randomVolumeSource->Update() );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( constantVolumeSource->Update() );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( randomProjectionsSource->Update() );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( constantProjectionsSource->Update() );

  // Geometry object
  typedef rtk::ThreeDCircularProjectionGeometry GeometryType;
  GeometryType::Pointer geometry = GeometryType::New();
  for(unsigned int noProj=0; noProj<NumberOfProjectionImages; noProj++)
    geometry->AddProjection(600., 1200., noProj*360./NumberOfProjectionImages);

  std::cout << "\n\n****** CUDA ray cast Forward projector ******" << std::endl;

  typedef rtk::CudaForwardProjectionImageFilter<OutputImageType, OutputImageType> ForwardProjectorType;
  ForwardProjectorType::Pointer fw = ForwardProjectorType::New();
  fw->SetInput(0, constantProjectionsSource->GetOutput());
  fw->SetInput(1, randomVolumeSource->GetOutput());
  fw->SetGeometry( geometry );
  TRY_AND_EXIT_ON_ITK_EXCEPTION( fw->Update() );

  std::cout << "\n\n****** CUDA ray cast Back projector ******" << std::endl;
  
  typedef rtk::CudaRayCastBackProjectionImageFilter BackProjectorType;
  BackProjectorType::Pointer bp = BackProjectorType::New();
  bp->SetInput(0, constantVolumeSource->GetOutput());
  bp->SetInput(1, randomProjectionsSource->GetOutput());
  bp->SetGeometry( geometry.GetPointer() );
  bp->SetNormalize(false);
  
  TRY_AND_EXIT_ON_ITK_EXCEPTION( bp->Update() );

  CheckScalarProducts<OutputImageType, OutputImageType>(randomVolumeSource->GetOutput(), bp->GetOutput(), randomProjectionsSource->GetOutput(), fw->GetOutput());
  std::cout << "\n\nTest PASSED! " << std::endl;

  return EXIT_SUCCESS;
}
