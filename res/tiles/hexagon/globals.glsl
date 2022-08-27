#include "/globals.glsl"

//some pixels of rect row 0 do not fall inside a hexagon.
//there we need to discard them
bool discardOutsideOfGridFragments(vec2 fragCoord, vec2 minBounds, vec2 maxBounds, vec2 rectSizeScreenSpace, int max_rect_col, int max_rect_row){
    
    // to get intervals from 0 to maxCoord, we map the original Point interval to maxCoord+1
    // If the current value = maxValue, we take the maxCoord instead
    int rectX = min(max_rect_col, mapInterval(fragCoord.x, minBounds.x, maxBounds.x, max_rect_col + 1));
    int rectY = min(max_rect_row, mapInterval(fragCoord.y, minBounds.y, maxBounds.y, max_rect_row + 1));

    if (rectY == 0 && mod(rectX, 3) != 2)
    {
        // rectangle left lower corner in space of points
        vec2 ll = vec2(rectX * rectSizeScreenSpace.x + minBounds.x, minBounds.y);
        vec2 a, b;
        //modX = 0
        if (mod(rectX, 3) == 0)
        {
            //Upper Left
            a = ll;
            b = vec2(ll.x + rectSizeScreenSpace.x / 2.0f, ll.y + rectSizeScreenSpace.y);
            if (pointLeftOfLine(a, b, fragCoord))
            {
                return true;
            }
        }
        // modX = 1
        else
        {
            //Upper Right
            a = vec2(ll.x + rectSizeScreenSpace.x / 2.0f, ll.y + rectSizeScreenSpace.y);
            b = vec2(ll.x + rectSizeScreenSpace.x, ll.y);
            if (pointLeftOfLine(a, b, fragCoord))
            {
                return true;
            }
        }
    }
    return false;
}

/*
p                   = point that is matched
max_rect_col        = maximum x coordinate in rectangle grid
max_rect_row        = minimum y coordinate in rectangle grid
rectWidth           = width of rectancle
rectHeight          = height of rectangle
minBounds           = minimum bounds of rectangle space
maxBounds           = maximum bounds of rectangle space
*/
vec2 matchPointWithHexagon(vec2 p, int max_rect_col,int max_rect_row, float rectWidth, float rectHeight, vec2 minBounds, vec2 maxBounds){
    
    // to get intervals from 0 to maxCoord, we map the original Point interval to maxCoord+1
    // If the current value = maxValue, we take the maxCoord instead
    int rectX = min(max_rect_col, mapInterval(p.x, minBounds.x, maxBounds.x, max_rect_col+1));
    int rectY = min(max_rect_row, mapInterval(p.y, minBounds.y, maxBounds.y, max_rect_row+1));

    // rectangle left lower corner in space of points
    vec2 ll = vec2(rectX * rectWidth + minBounds.x, rectY * rectHeight + minBounds.y);
    // points defining a line
    vec2 a, b;

    // calculate hexagon index from rectangle index --------------------
    int hexX, hexY, modX, modY;
    
    // get modulo values
    modX = int(mod(rectX,3));
    modY = int(mod(rectY,2));

    //calculate initial X index
    hexX = int(rectX/3) * 2 + modX - 1;
    // if modX != 2 we need to check if we are in correct hexagon
    if(modX != 2){
        if(modX == 0){
            if(modY == 0){
                //Upper Left
                a = ll;
                b = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                if(pointLeftOfLine(a,b,p)){
                    hexX++;
                }
            }
            //modY = 1
            else{
                //Lower Left
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x, ll.y + rectHeight);
                if(pointLeftOfLine(a,b,p)){
                    hexX++;
                }
            }
        }
        //modX = 1
        else{

            if(modY == 0){
                //Upper Right
                a = vec2(ll.x + rectWidth, ll.y);
                b = vec2(ll.x + rectWidth/2.0f, ll.y + rectHeight);
                if(pointLeftOfLine(a,b,p)){
                    hexX++;
                }
            }
            //modY = 1
            else{
                //Lower Right
                a = vec2(ll.x + rectWidth/2.0f, ll.y);
                b = vec2(ll.x + rectWidth, ll.y + rectHeight);
                if(pointLeftOfLine(a,b,p)){
                    hexX++;
                }
            }
        }
    }

    // Y-Coordinate
    hexY = int((rectY - ( 1 - mod(hexX, 2)))/2);

    return vec2(hexX, hexY);
}