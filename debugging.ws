<?xml version="1.0"?>
<root xmlns="http://www.vips.ecs.soton.ac.uk/nip/7.26.3">
  <Workspace window_x="0" window_y="0" window_width="1270" window_height="809" filename="/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/debugging.ws" view="WORKSPACE_MODE_REGULAR" scale="1" offset="0" lpane_position="100" lpane_open="false" rpane_position="400" rpane_open="false" local_defs="// private definitions for this workspace&#10;" name="debugging" caption="Default empty workspace">
    <Column x="0" y="0" open="true" selected="false" sform="false" next="4" name="A" caption="T">
      <Subcolumn vislevel="3">
        <Row popup="false" name="A1">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="734" image_top="674" image_mag="-2" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/terminals.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="A2">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="47" window_y="32" window_width="737" window_height="757" image_left="894" image_top="628" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_transform_item.Flip_item.Top_bottom_item.action A1"/>
          </Rhs>
        </Row>
        <Row popup="false" name="A3">
          <Rhs vislevel="1" flags="4">
            <iText formula="Math_stats_item.Max_item.action A2"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="0" y="249" open="true" selected="false" sform="false" next="4" name="B" caption="down">
      <Subcolumn vislevel="3">
        <Row popup="false" name="B1">
          <Rhs vislevel="1" flags="1">
            <iImage image_left="0" image_top="0" image_mag="0" show_status="false" show_paintbox="false" show_convert="false" show_rulers="false" scale="0" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/down.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="B2">
          <Rhs vislevel="0" flags="4">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="357" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="B1*2000"/>
          </Rhs>
        </Row>
        <Row popup="false" name="B3">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="346" window_y="12" window_width="750" window_height="750" image_left="734" image_top="674" image_mag="-2" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_transform_item.Flip_item.Top_bottom_item.action B2"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="0" y="498" open="true" selected="false" sform="false" next="4" name="D" caption="up">
      <Subcolumn vislevel="3">
        <Row popup="false" name="D1">
          <Rhs vislevel="1" flags="1">
            <iImage image_left="0" image_top="0" image_mag="0" show_status="false" show_paintbox="false" show_convert="false" show_rulers="false" scale="0" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/up.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="D2">
          <Rhs vislevel="0" flags="4">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="357" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="D1*2000"/>
          </Rhs>
        </Row>
        <Row popup="false" name="D3">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="283" window_y="0" window_width="750" window_height="750" image_left="89" image_top="82" image_mag="4" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_transform_item.Flip_item.Top_bottom_item.action D2"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="876" y="0" open="true" selected="false" sform="false" next="4" name="E" caption="right">
      <Subcolumn vislevel="3">
        <Row popup="false" name="E1">
          <Rhs vislevel="1" flags="1">
            <iImage image_left="0" image_top="0" image_mag="0" show_status="false" show_paintbox="false" show_convert="false" show_rulers="false" scale="0" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/right.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="E2">
          <Rhs vislevel="0" flags="4">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="357" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="E1*2000"/>
          </Rhs>
        </Row>
        <Row popup="false" name="E3">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="627" window_y="92" window_width="750" window_height="750" image_left="357" image_top="613" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_transform_item.Flip_item.Top_bottom_item.action E2"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="876" y="249" open="true" selected="false" sform="false" next="4" name="F" caption="right">
      <Subcolumn vislevel="3">
        <Row popup="false" name="F1">
          <Rhs vislevel="1" flags="1">
            <iImage image_left="0" image_top="0" image_mag="0" show_status="false" show_paintbox="false" show_convert="false" show_rulers="false" scale="0" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/left.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="F2">
          <Rhs vislevel="0" flags="4">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="357" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="F1*2000"/>
          </Rhs>
        </Row>
        <Row popup="false" name="F3">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="583" image_top="491" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_transform_item.Flip_item.Top_bottom_item.action F2"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="434" y="0" open="true" selected="false" sform="false" next="4" name="G" caption="second">
      <Subcolumn vislevel="3">
        <Row popup="false" name="G1">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="734" image_top="674" image_mag="-2" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/second.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="G2">
          <Rhs vislevel="0" flags="4">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="357" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="G1*2000"/>
          </Rhs>
        </Row>
        <Row popup="false" name="G3">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="596" window_y="9" window_width="750" window_height="750" image_left="762" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/first.png&quot;"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="434" y="249" open="true" selected="true" sform="false" next="5" name="H" caption="segment">
      <Subcolumn vislevel="3">
        <Row popup="false" name="H1">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="922" image_top="632" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/segment.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="H2">
          <Rhs vislevel="0" flags="4">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="357" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="H1*255"/>
          </Rhs>
        </Row>
        <Row popup="false" name="H3">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="661" window_y="95" window_width="750" window_height="750" image_left="947" image_top="626" image_mag="2" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_transform_item.Flip_item.Top_bottom_item.action H2"/>
          </Rhs>
        </Row>
        <Row popup="false" name="H4">
          <Rhs vislevel="2" flags="5">
            <iImage window_x="694" window_y="81" window_width="697" window_height="590" image_left="340" image_top="257" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0">
              <Row name="x">
                <Rhs vislevel="0" flags="4">
                  <iText/>
                </Rhs>
              </Row>
              <Row name="super">
                <Rhs vislevel="0" flags="4">
                  <iImage image_left="0" image_top="0" image_mag="0" show_status="false" show_paintbox="false" show_convert="false" show_rulers="false" scale="0" offset="0" falsecolour="false" type="true"/>
                  <Subcolumn vislevel="0"/>
                  <iText/>
                </Rhs>
              </Row>
              <Row name="xfactor">
                <Rhs vislevel="1" flags="1">
                  <Expression caption="Horizontal scale factor"/>
                  <Subcolumn vislevel="0">
                    <Row name="caption">
                      <Rhs vislevel="0" flags="4">
                        <iText/>
                      </Rhs>
                    </Row>
                    <Row name="expr">
                      <Rhs vislevel="0" flags="4">
                        <iText formula="0.5"/>
                      </Rhs>
                    </Row>
                    <Row name="super">
                      <Rhs vislevel="1" flags="4">
                        <Subcolumn vislevel="0"/>
                        <iText/>
                      </Rhs>
                    </Row>
                  </Subcolumn>
                  <iText/>
                </Rhs>
              </Row>
              <Row name="yfactor">
                <Rhs vislevel="1" flags="1">
                  <Expression caption="Vertical scale factor"/>
                  <Subcolumn vislevel="0">
                    <Row name="caption">
                      <Rhs vislevel="0" flags="4">
                        <iText/>
                      </Rhs>
                    </Row>
                    <Row name="expr">
                      <Rhs vislevel="0" flags="4">
                        <iText formula="0.5"/>
                      </Rhs>
                    </Row>
                    <Row name="super">
                      <Rhs vislevel="1" flags="4">
                        <Subcolumn vislevel="0"/>
                        <iText/>
                      </Rhs>
                    </Row>
                  </Subcolumn>
                  <iText/>
                </Rhs>
              </Row>
              <Row name="interp">
                <Rhs vislevel="2" flags="6">
                  <Subcolumn vislevel="1">
                    <Row name="default">
                      <Rhs vislevel="0" flags="4">
                        <iText/>
                      </Rhs>
                    </Row>
                    <Row name="super">
                      <Rhs vislevel="1" flags="4">
                        <Subcolumn vislevel="0"/>
                        <iText/>
                      </Rhs>
                    </Row>
                    <Row name="interp">
                      <Rhs vislevel="1" flags="1">
                        <Option caption="Interpolation" labelsn="6" labels0="Nearest neighbour" labels1="Bilinear" labels2="Bicubic" labels3="Upsize: reduced halo bicubic (LBB)" labels4="Upsharp: reduced halo bicubic with edge sharpening (Nohalo)" labels5="Upsmooth: quadratic B-splines with jaggy reduction (VSQBS)" value="0"/>
                        <Subcolumn vislevel="0"/>
                        <iText/>
                      </Rhs>
                    </Row>
                  </Subcolumn>
                  <iText/>
                </Rhs>
              </Row>
            </Subcolumn>
            <iText formula="Image_transform_item.Resize_item.Scale_item.action H3"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="434" y="629" open="true" selected="false" sform="false" next="7" name="C" caption="masks">
      <Subcolumn vislevel="3">
        <Row popup="false" name="C1">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="367" image_top="337" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/mask.png&quot;"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
    <Column x="876" y="498" open="true" selected="false" sform="false" next="4" name="I" caption="costs">
      <Subcolumn vislevel="3">
        <Row popup="false" name="I1">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="157" window_y="62" window_width="654" window_height="519" image_left="309" image_top="212" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_file &quot;/Users/emmanueldurand/Documents/Dev/git/base/projects/kinect/release/kinect.app/Contents/Resources/costs.png&quot;"/>
          </Rhs>
        </Row>
        <Row popup="false" name="I2">
          <Rhs vislevel="0" flags="4">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="357" image_top="328" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="I1*2000"/>
          </Rhs>
        </Row>
        <Row popup="false" name="I3">
          <Rhs vislevel="1" flags="1">
            <iImage window_x="67" window_y="20" window_width="750" window_height="750" image_left="367" image_top="337" image_mag="1" show_status="true" show_paintbox="false" show_convert="false" show_rulers="false" scale="1" offset="0" falsecolour="false" type="true"/>
            <Subcolumn vislevel="0"/>
            <iText formula="Image_transform_item.Flip_item.Top_bottom_item.action I2"/>
          </Rhs>
        </Row>
      </Subcolumn>
    </Column>
  </Workspace>
</root>



