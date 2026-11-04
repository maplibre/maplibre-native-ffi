#import <Foundation/Foundation.h>

// Installs the binding and the surface registry when JavaScript first loads
// the module, and cleans the binding up before React Native destroys the
// runtime. Both run on the JavaScript thread.
@interface MLNExampleHost : NSObject
@end
