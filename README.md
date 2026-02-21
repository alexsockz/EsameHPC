THE PLAN

1st working version with a matrix that gets explored as normal
2nd version, the 2 "external" layers of the matrix, the incoming data and the border data that needs to be sent will be before the rest of the internal data, which will then respect the data allocation like normal matrix
3rd version the data will be in 2 equivalent matrices, but organised so one is width-explorable and one is height-explorable such that you can decompose the calculations of the final stencil in vertical and horizontal


TO do this it will be necessary a function that returns the correct index