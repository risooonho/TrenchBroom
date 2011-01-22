//
//  TextureViewManager.h
//  TrenchBroom
//
//  Created by Kristian Duske on 22.01.11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class Texture;

@interface TextureViewLayout : NSObject {
    @private
    NSMutableArray* rows;
    NSMutableArray* textures;
    float width;
    float innerMargin;
    float outerMargin;
}

- (id)initWithWidth:(float)theWidth innerMargin:(float)theInnerMargin outerMargin:(float)theOuterMargin;

- (void)addTexture:(Texture *)theTexture;
- (void)addTextures:(NSArray *)theTextures;
- (void)removeAllTextures;

- (void)setWidth:(float)theWidth;

- (void)layout;

- (float)height;
- (NSArray *)rows;
- (NSArray *)rowsInY:(float)y height:(float)height;

@end
