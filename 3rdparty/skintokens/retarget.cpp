#include <skintokens/skintokens.hpp>

#include "internal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional.hpp>
#include <string_view>
#include <unordered_map>

namespace skintokens {
namespace {

quat qmul(quat a, quat b) {
    return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,
            a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,
            a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,
            a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};
}

quat qnorm(quat value) {
    const float length=std::sqrt(value.x*value.x+value.y*value.y+value.z*value.z+value.w*value.w);
    return length>1.0e-12F ? quat{value.x/length,value.y/length,value.z/length,value.w/length} : quat{};
}

vec3 rotate(quat q, vec3 v) {
    q=qnorm(q);
    const vec3 u{q.x,q.y,q.z};
    const float dot=u.x*v.x+u.y*v.y+u.z*v.z;
    const vec3 cross{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
    const float uu=u.x*u.x+u.y*u.y+u.z*u.z;
    return {2.0F*dot*u.x+(q.w*q.w-uu)*v.x+2.0F*q.w*cross.x,
            2.0F*dot*u.y+(q.w*q.w-uu)*v.y+2.0F*q.w*cross.y,
            2.0F*dot*u.z+(q.w*q.w-uu)*v.z+2.0F*q.w*cross.z};
}

vec3 add(vec3 a, vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
vec3 sub(vec3 a, vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
vec3 scale(vec3 a, float s) { return {a.x*s,a.y*s,a.z*s}; }
float length(vec3 a) { return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z); }
vec3 normalized(vec3 a) { const float l=length(a); return l>1.0e-8F ? scale(a,1.0F/l) : vec3{1.0F,0.0F,0.0F}; }
float distance(vec3 a, vec3 b) { return length(sub(a,b)); }
constexpr std::array<std::string_view,30> soma_names{{
    "Hips","Spine1","Spine2","Chest","Neck1","Neck2","Head","Jaw","LeftEye","RightEye",
    "LeftShoulder","LeftArm","LeftForeArm","LeftHand","LeftHandThumbEnd","LeftHandMiddleEnd",
    "RightShoulder","RightArm","RightForeArm","RightHand","RightHandThumbEnd","RightHandMiddleEnd",
    "LeftLeg","LeftShin","LeftFoot","LeftToeBase","RightLeg","RightShin","RightFoot","RightToeBase"
}};

result<void> validate(const skeleton & rig) {
    const std::size_t count=rig.names.size();
    if (count==0U || count>256U || rig.parents.size()!=count || rig.rest_positions.size()!=count)
        return tl::unexpected(detail::fail(error_code::invalid_argument,"skeleton arrays must contain 1..256 matching joints"));
    std::size_t roots=0U;
    for (std::size_t i=0;i<count;++i) {
        if (rig.parents[i]<0) ++roots;
        else if (static_cast<std::size_t>(rig.parents[i])>=i)
            return tl::unexpected(detail::fail(error_code::invalid_argument,"skeleton parents must precede children"));
    }
    if (roots!=1U) return tl::unexpected(detail::fail(error_code::invalid_argument,"skeleton must contain exactly one root"));
    return {};
}

result<void> validate(const motion & value) {
    auto rig=validate(value.rig);
    if (!rig) return rig;
    if (value.frames==0U || value.root_translations.size()!=value.frames ||
        value.local_rotations.size()!=value.frames*value.rig.names.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument,"motion arrays are incomplete"));
    return {};
}

nonstd::optional<std::size_t> find(const skeleton & rig, std::string_view name) {
    const auto value=std::find(rig.names.begin(),rig.names.end(),name);
    if (value==rig.names.end()) return std::nullopt;
    return static_cast<std::size_t>(value-rig.names.begin());
}

struct body_pair { std::string_view target; std::string_view source; };
constexpr std::array<body_pair,22> body_pairs{{
    {"mixamorig:Hips","Hips"},{"mixamorig:Spine","Spine1"},
    {"mixamorig:Spine1","Spine2"},{"mixamorig:Spine2","Chest"},
    {"mixamorig:Neck","Neck1"},{"mixamorig:Head","Head"},
    {"mixamorig:LeftShoulder","LeftShoulder"},{"mixamorig:LeftArm","LeftArm"},
    {"mixamorig:LeftForeArm","LeftForeArm"},{"mixamorig:LeftHand","LeftHand"},
    {"mixamorig:RightShoulder","RightShoulder"},{"mixamorig:RightArm","RightArm"},
    {"mixamorig:RightForeArm","RightForeArm"},{"mixamorig:RightHand","RightHand"},
    {"mixamorig:LeftUpLeg","LeftLeg"},{"mixamorig:LeftLeg","LeftShin"},
    {"mixamorig:LeftFoot","LeftFoot"},{"mixamorig:LeftToeBase","LeftToeBase"},
    {"mixamorig:RightUpLeg","RightLeg"},{"mixamorig:RightLeg","RightShin"},
    {"mixamorig:RightFoot","RightFoot"},{"mixamorig:RightToeBase","RightToeBase"},
}};

const std::array<std::string_view,30> finger_names{{
    "mixamorig:LeftHandThumb1","mixamorig:LeftHandThumb2","mixamorig:LeftHandThumb3",
    "mixamorig:LeftHandIndex1","mixamorig:LeftHandIndex2","mixamorig:LeftHandIndex3",
    "mixamorig:LeftHandMiddle1","mixamorig:LeftHandMiddle2","mixamorig:LeftHandMiddle3",
    "mixamorig:LeftHandRing1","mixamorig:LeftHandRing2","mixamorig:LeftHandRing3",
    "mixamorig:LeftHandPinky1","mixamorig:LeftHandPinky2","mixamorig:LeftHandPinky3",
    "mixamorig:RightHandIndex1","mixamorig:RightHandIndex2","mixamorig:RightHandIndex3",
    "mixamorig:RightHandThumb1","mixamorig:RightHandThumb2","mixamorig:RightHandThumb3",
    "mixamorig:RightHandMiddle1","mixamorig:RightHandMiddle2","mixamorig:RightHandMiddle3",
    "mixamorig:RightHandRing1","mixamorig:RightHandRing2","mixamorig:RightHandRing3",
    "mixamorig:RightHandPinky1","mixamorig:RightHandPinky2","mixamorig:RightHandPinky3",
}};

struct map_entry { std::size_t target; std::size_t source; };
result<std::vector<map_entry>> semantic_map(const skeleton & source,const skeleton & target) {
    std::vector<map_entry> output;
    output.reserve(body_pairs.size());
    for (const auto & pair:body_pairs) {
        const auto s=find(source,pair.source),t=find(target,pair.target);
        if (!s) return tl::unexpected(detail::fail(error_code::invalid_argument,
            "SOMA30 skeleton is missing joint "+std::string{pair.source}));
        if (!t) return tl::unexpected(detail::fail(error_code::invalid_argument,
            "Mixamo52 skeleton is missing joint "+std::string{pair.target}));
        output.push_back({*t,*s});
    }
    return output;
}

std::pair<vec3,float> center_scale(const skeleton & rig) {
    vec3 low=rig.rest_positions.front(),high=low;
    for (const auto p:rig.rest_positions) {
        low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
        high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};
    }
    return {{(low.x+high.x)*.5F,(low.y+high.y)*.5F,(low.z+high.z)*.5F},
            std::max({high.x-low.x,high.y-low.y,high.z-low.z})};
}

std::vector<vec3> posed_joints(const motion & value,std::size_t frame) {
    const std::size_t count=value.rig.names.size();
    std::vector<vec3> positions(count);
    std::vector<quat> rotations(count);
    for (std::size_t joint=0;joint<count;++joint) {
        const auto parent=value.rig.parents[joint];
        const quat local=value.local_rotations[frame*count+joint];
        if (parent<0) {
            rotations[joint]=qnorm(local);
            positions[joint]=value.root_translations[frame];
        } else {
            const auto p=static_cast<std::size_t>(parent);
            rotations[joint]=qnorm(qmul(rotations[p],local));
            positions[joint]=add(positions[p],rotate(rotations[p],sub(value.rig.rest_positions[joint],value.rig.rest_positions[p])));
        }
    }
    return positions;
}

void add_errors(const motion & source,const motion & target,nonstd::span<const map_entry> mapping,
                double & sum,float & maximum,std::size_t & samples) {
    const auto anatomical_scale=[](const skeleton & rig,std::string_view head_name,
                                   std::string_view left_toe_name,std::string_view right_toe_name) {
        const auto head=find(rig,head_name),left=find(rig,left_toe_name),right=find(rig,right_toe_name);
        if (!head || !left || !right) return center_scale(rig).second;
        const vec3 feet=scale(add(rig.rest_positions[*left],rig.rest_positions[*right]),.5F);
        return std::max(distance(rig.rest_positions[*head],feet),1.0e-8F);
    };
    const float source_scale=anatomical_scale(source.rig,"Head","LeftToeBase","RightToeBase");
    const float target_scale=anatomical_scale(target.rig,"mixamorig:Head","mixamorig:LeftToeBase","mixamorig:RightToeBase");
    for (std::size_t frame=0;frame<source.frames;++frame) {
        const auto source_pose=posed_joints(source,frame),target_pose=posed_joints(target,frame);
        const vec3 source_root=source_pose.front(),target_root=target_pose.front();
        for (const auto item:mapping) {
            const auto a=scale(sub(source_pose[item.source],source_root),1.0F/source_scale);
            const auto b=scale(sub(target_pose[item.target],target_root),1.0F/target_scale);
            const float error=distance(a,b);
            sum+=error;maximum=std::max(maximum,error);++samples;
        }
    }
}

} // namespace

result<skeleton> make_mixamo52_rig(const skeleton & source) {
    auto valid=validate(source);
    if (!valid) return tl::unexpected(valid.error());
    skeleton output;
    output.names.reserve(52U);output.parents.reserve(52U);output.rest_positions.reserve(52U);
    const std::array<std::int32_t,22> parents{{-1,0,1,2,3,4,3,6,7,8,3,10,11,12,0,14,15,16,0,18,19,20}};
    for (std::size_t i=0;i<body_pairs.size();++i) {
        const auto source_joint=find(source,body_pairs[i].source);
        if (!source_joint) return tl::unexpected(detail::fail(error_code::invalid_argument,
            "SOMA30 skeleton is missing joint "+std::string{body_pairs[i].source}));
        output.names.emplace_back(body_pairs[i].target);
        output.parents.push_back(parents[i]);
        output.rest_positions.push_back(source.rest_positions[*source_joint]);
    }
    const auto add_hand=[&](bool left,nonstd::span<const std::string_view> names) -> result<void> {
        const std::string side=left?"Left":"Right";
        const auto hand=find(source,side+"Hand"),thumb=find(source,side+"HandThumbEnd"),middle=find(source,side+"HandMiddleEnd");
        if (!hand || !thumb || !middle) return tl::unexpected(detail::fail(error_code::invalid_argument,
            "SOMA30 skeleton is missing "+side+" hand endpoints"));
        const vec3 origin=source.rest_positions[*hand];
        const vec3 middle_vector=sub(source.rest_positions[*middle],origin);
        const float hand_length=std::max(length(middle_vector),1.0e-5F);
        const vec3 forward=normalized(middle_vector);
        const vec3 thumb_vector=sub(source.rest_positions[*thumb],origin);
        vec3 lateral=sub(thumb_vector,scale(forward,thumb_vector.x*forward.x+thumb_vector.y*forward.y+thumb_vector.z*forward.z));
        if (length(lateral)<hand_length*.05F) lateral={left?0.0F:0.0F,0.0F,left?-1.0F:1.0F};
        lateral=normalized(lateral);
        const std::array<float,5> along{{.78F,.96F,1.0F,.92F,.82F}};
        const std::array<float,5> across{{.42F,.14F,0.0F,-.13F,-.27F}};
        const std::size_t hand_target=*find(output,left?"mixamorig:LeftHand":"mixamorig:RightHand");
        const std::array<std::size_t,5> semantic_order=left ?
            std::array<std::size_t,5>{{0U,1U,2U,3U,4U}} :
            std::array<std::size_t,5>{{1U,0U,2U,3U,4U}};
        for (std::size_t ordered=0;ordered<5U;++ordered) {
            const std::size_t finger=semantic_order[ordered];
            vec3 endpoint=add(origin,add(scale(forward,hand_length*along[finger]),scale(lateral,hand_length*across[finger])));
            if (finger==0U) endpoint=source.rest_positions[*thumb];
            if (finger==2U) endpoint=source.rest_positions[*middle];
            std::int32_t parent=static_cast<std::int32_t>(hand_target);
            for (std::size_t segment=0;segment<3U;++segment) {
                output.names.emplace_back(names[ordered*3U+segment]);
                output.parents.push_back(parent);
                output.rest_positions.push_back(add(origin,scale(sub(endpoint,origin),static_cast<float>(segment+1U)/3.0F)));
                parent=static_cast<std::int32_t>(output.names.size()-1U);
            }
        }
        return {};
    };
    auto left=add_hand(true,nonstd::span<const std::string_view>{finger_names}.first<15>());
    if (!left) return tl::unexpected(left.error());
    // The published Mixamo order puts right index before right thumb.
    auto right=add_hand(false,nonstd::span<const std::string_view>{finger_names}.subspan(15U));
    if (!right) return tl::unexpected(right.error());
    // Upstream's arbitrary-GLB path labels the asset "articulation", so its
    // Mixamo parts table is not selected. Blender's armature importer exposes
    // bones in hierarchy preorder instead. Skin-code group i is positional;
    // retaining the YAML's body-then-hands list here would decode hand codes
    // onto unrelated body joints even though the named retarget is correct.
    std::vector<std::size_t> order;
    order.reserve(output.names.size());
    const auto visit = [&](auto && self,std::size_t parent) -> void {
        order.push_back(parent);
        for (std::size_t child=0;child<output.parents.size();++child)
            if (output.parents[child]==static_cast<std::int32_t>(parent)) self(self,child);
    };
    visit(visit,0U);
    if (order.size()!=output.names.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument,"Mixamo52 hierarchy is disconnected"));
    std::vector<std::size_t> old_to_new(order.size());
    for (std::size_t index=0;index<order.size();++index) old_to_new[order[index]]=index;
    skeleton arranged;
    arranged.names.reserve(order.size());arranged.parents.reserve(order.size());arranged.rest_positions.reserve(order.size());
    for (const auto old:order) {
        arranged.names.push_back(output.names[old]);
        arranged.rest_positions.push_back(output.rest_positions[old]);
        arranged.parents.push_back(output.parents[old]<0 ? -1 :
            static_cast<std::int32_t>(old_to_new[static_cast<std::size_t>(output.parents[old])]));
    }
    return arranged;
}

result<motion> retarget_soma30_to_mixamo52(const motion & animation,const skeleton & target) {
    auto valid_source=validate(animation);if (!valid_source) return tl::unexpected(valid_source.error());
    auto valid_target=validate(target);if (!valid_target) return tl::unexpected(valid_target.error());
    auto mapping=semantic_map(animation.rig,target);if (!mapping) return tl::unexpected(mapping.error());
    motion output;
    output.frames=animation.frames;output.frames_per_second=animation.frames_per_second;output.rig=target;
    output.root_translations=animation.root_translations;
    output.local_rotations.assign(output.frames*target.names.size(),{});
    const auto source_neck2=find(animation.rig,"Neck2");
    const auto target_neck=find(target,"mixamorig:Neck");
    for (std::size_t frame=0;frame<output.frames;++frame) {
        for (const auto item:*mapping)
            output.local_rotations[frame*target.names.size()+item.target]=
                animation.local_rotations[frame*animation.rig.names.size()+item.source];
        if (source_neck2 && target_neck) {
            auto & neck=output.local_rotations[frame*target.names.size()+*target_neck];
            neck=qnorm(qmul(neck,animation.local_rotations[frame*animation.rig.names.size()+*source_neck2]));
        }
    }
    return output;
}

result<retarget_report> validate_soma30_to_mixamo52(const motion & animation,const skeleton & target) {
    auto valid=validate(animation);if (!valid) return tl::unexpected(valid.error());
    auto mapping=semantic_map(animation.rig,target);if (!mapping) return tl::unexpected(mapping.error());
    auto transferred=retarget_soma30_to_mixamo52(animation,target);
    if (!transferred) return tl::unexpected(transferred.error());
    retarget_report report;
    double sum=0.0;std::size_t samples=0U;
    add_errors(animation,*transferred,*mapping,sum,report.motion_max_position_error,samples);
    report.motion_mean_position_error=samples?static_cast<float>(sum/static_cast<double>(samples)):0.0F;

    constexpr float sine=.25881904510252074F,cosine=.9659258262890683F; // 30 degrees / 2
    const std::array<quat,6> probes{{{sine,0,0,cosine},{-sine,0,0,cosine},
        {0,sine,0,cosine},{0,-sine,0,cosine},{0,0,sine,cosine},{0,0,-sine,cosine}}};
    const auto source_neck2=find(animation.rig,"Neck2");
    double isolated_sum=0.0;std::size_t isolated_samples=0U;
    for (const auto item:*mapping) {
        // Neck is a deliberate two-to-one collapse and is exercised through
        // both source joints; all other probes are exact semantic pairs.
        std::array<std::size_t,2> drivers{{item.source,item.source}};
        std::size_t driver_count=1U;
        if (target.names[item.target]=="mixamorig:Neck" && source_neck2) { drivers[1]=*source_neck2;driver_count=2U; }
        for (std::size_t d=0;d<driver_count;++d) for (const auto probe:probes) {
            motion isolated;
            isolated.frames=1U;isolated.frames_per_second=30.0F;isolated.rig=animation.rig;
            isolated.root_translations={animation.rig.rest_positions.front()};
            isolated.local_rotations.assign(animation.rig.names.size(),{});
            isolated.local_rotations[drivers[d]]=probe;
            auto result=retarget_soma30_to_mixamo52(isolated,target);
            if (!result) return tl::unexpected(result.error());
            add_errors(isolated,*result,*mapping,isolated_sum,report.isolated_max_position_error,isolated_samples);
            ++report.isolated_cases;
        }
    }
    report.isolated_mean_position_error=isolated_samples?
        static_cast<float>(isolated_sum/static_cast<double>(isolated_samples)):0.0F;

    const auto source_left=find(animation.rig,"LeftFoot"),source_right=find(animation.rig,"RightFoot");
    const auto target_left=find(target,"mixamorig:LeftFoot"),target_right=find(target,"mixamorig:RightFoot");
    double velocity_difference=0.0,velocity_reference=0.0;
    if (animation.frames>1U && source_left && source_right && target_left && target_right) {
        auto previous_source=posed_joints(animation,0U),previous_target=posed_joints(*transferred,0U);
        for (std::size_t frame=1U;frame<animation.frames;++frame) {
            auto current_source=posed_joints(animation,frame),current_target=posed_joints(*transferred,frame);
            for (const auto pair:std::array<std::pair<std::size_t,std::size_t>,2>{{{*source_left,*target_left},{*source_right,*target_right}}}) {
                const vec3 a=sub(current_source[pair.first],previous_source[pair.first]);
                const vec3 b=sub(current_target[pair.second],previous_target[pair.second]);
                velocity_difference+=distance(a,b);velocity_reference+=length(a);
            }
            previous_source=std::move(current_source);previous_target=std::move(current_target);
        }
    }
    report.foot_velocity_relative_error=static_cast<float>(velocity_difference/std::max(velocity_reference,1.0e-8));
    return report;
}

result<retarget_report> compare_soma30_to_mixamo52(const motion & source,const motion & target) {
    auto valid_source=validate(source);if (!valid_source) return tl::unexpected(valid_source.error());
    auto valid_target=validate(target);if (!valid_target) return tl::unexpected(valid_target.error());
    if (source.frames!=target.frames)
        return tl::unexpected(detail::fail(error_code::invalid_argument,"retarget clips have different frame counts"));
    auto mapping=semantic_map(source.rig,target.rig);if (!mapping) return tl::unexpected(mapping.error());
    retarget_report report;
    double sum=0.0;std::size_t samples=0U;
    add_errors(source,target,*mapping,sum,report.motion_max_position_error,samples);
    report.motion_mean_position_error=samples?static_cast<float>(sum/static_cast<double>(samples)):0.0F;
    const auto source_left=find(source.rig,"LeftFoot"),source_right=find(source.rig,"RightFoot");
    const auto target_left=find(target.rig,"mixamorig:LeftFoot"),target_right=find(target.rig,"mixamorig:RightFoot");
    double velocity_difference=0.0,velocity_reference=0.0;
    if (source.frames>1U && source_left && source_right && target_left && target_right) {
        auto previous_source=posed_joints(source,0U),previous_target=posed_joints(target,0U);
        for (std::size_t frame=1U;frame<source.frames;++frame) {
            auto current_source=posed_joints(source,frame),current_target=posed_joints(target,frame);
            for (const auto pair:std::array<std::pair<std::size_t,std::size_t>,2>{{{*source_left,*target_left},{*source_right,*target_right}}}) {
                const vec3 a=sub(current_source[pair.first],previous_source[pair.first]);
                const vec3 b=sub(current_target[pair.second],previous_target[pair.second]);
                velocity_difference+=distance(a,b);velocity_reference+=length(a);
            }
            previous_source=std::move(current_source);previous_target=std::move(current_target);
        }
    }
    report.foot_velocity_relative_error=static_cast<float>(velocity_difference/std::max(velocity_reference,1.0e-8));
    return report;
}

result<humanoid_mapping> match_generated_humanoid(
    const skeleton & generated,const humanoid_match_options & options) {
    auto valid=validate(generated);if (!valid) return tl::unexpected(valid.error());
    if (!std::isfinite(options.minimum_confidence) || options.minimum_confidence<0.0F ||
        options.minimum_confidence>1.0F)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "humanoid minimum confidence must be between zero and one"));

    const std::size_t count=generated.names.size();
    std::vector<std::vector<std::size_t>> children(count);
    std::size_t root=count;
    for (std::size_t joint=0;joint<count;++joint) {
        if (generated.parents[joint]<0) root=joint;
        else children[static_cast<std::size_t>(generated.parents[joint])].push_back(joint);
    }
    if (root==count || children[root].size()!=3U)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated skeleton does not have a humanoid hips/trunk/two-leg root"));

    const auto path_to_branch=[&](std::size_t start) {
        std::vector<std::size_t> path;
        std::size_t cursor=start;
        for (std::size_t guard=0;guard<count;++guard) {
            path.push_back(cursor);
            if (children[cursor].size()!=1U) break;
            cursor=children[cursor].front();
        }
        return path;
    };
    const auto fixed_chain=[&](std::size_t start,std::size_t wanted) -> nonstd::optional<std::vector<std::size_t>> {
        std::vector<std::size_t> path;path.reserve(wanted);
        std::size_t cursor=start;
        for (std::size_t index=0;index<wanted;++index) {
            path.push_back(cursor);
            if (index+1U<wanted) {
                if (children[cursor].size()!=1U) return std::nullopt;
                cursor=children[cursor].front();
            }
        }
        return path;
    };
    const auto subtree_size=[&](auto && self,std::size_t joint) -> std::size_t {
        std::size_t total=1U;
        for (const auto child:children[joint]) total+=self(self,child);
        return total;
    };

    nonstd::optional<std::vector<std::size_t>> trunk;
    std::size_t trunk_root=count;
    for (const auto candidate:children[root]) {
        auto path=path_to_branch(candidate);
        if (!path.empty() && children[path.back()].size()>=3U) {
            if (trunk) return tl::unexpected(detail::fail(error_code::invalid_argument,
                "generated skeleton has more than one possible humanoid trunk"));
            trunk=std::move(path);trunk_root=candidate;
        }
    }
    if (!trunk || trunk->size()<3U || trunk->size()>5U || children[trunk->back()].size()!=3U)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated skeleton has no supported humanoid spine/chest split"));
    const std::size_t chest=trunk->back();

    std::vector<std::vector<std::size_t>> arm_candidates;
    std::vector<std::size_t> arm_starts;
    for (const auto child:children[chest]) {
        auto chain=fixed_chain(child,4U);
        if (chain) { arm_starts.push_back(child);arm_candidates.push_back(std::move(*chain)); }
    }
    if (arm_candidates.size()!=2U)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated skeleton does not have two shoulder/arm/forearm/hand chains"));
    std::size_t head_start=count;
    for (const auto child:children[chest])
        if (std::find(arm_starts.begin(),arm_starts.end(),child)==arm_starts.end()) head_start=child;
    if (head_start==count)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated skeleton has no neck/head continuation"));
    auto head=path_to_branch(head_start);
    if (head.size()<2U || children[head.back()].size()!=0U)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated skeleton has no supported terminal neck/head chain"));

    std::vector<std::vector<std::size_t>> legs;
    for (const auto child:children[root]) {
        if (child==trunk_root) continue;
        auto chain=fixed_chain(child,4U);
        if (!chain) return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated skeleton has an incomplete leg chain"));
        legs.push_back(std::move(*chain));
    }
    if (legs.size()!=2U)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated skeleton does not have two leg chains"));

    const auto left_first=[](const auto & pair,const skeleton & rig) {
        // VROID/SkinTokens and Kimodo use +X for the character's anatomical
        // left (the viewer sees it on screen-right in a front view).
        return rig.rest_positions[pair[0].front()].x>rig.rest_positions[pair[1].front()].x;
    };
    if (!left_first(arm_candidates,generated)) std::swap(arm_candidates[0],arm_candidates[1]);
    if (!left_first(legs,generated)) std::swap(legs[0],legs[1]);

    humanoid_mapping output;
    output.soma_to_generated.fill(-1);
    output.generated_roles.assign(count,semantic_role::unmapped);
    const auto assign=[&](std::size_t soma,std::size_t joint,semantic_role role) {
        output.soma_to_generated[soma]=static_cast<std::int32_t>(joint);
        output.generated_roles[joint]=role;
    };
    assign(0U,root,semantic_role::hips);
    const auto trunk_at=[&](std::size_t ordinal) {
        if (ordinal==0U) return trunk->front();
        if (ordinal==2U) return trunk->back();
        return (*trunk)[(trunk->size()-1U)/2U];
    };
    assign(1U,trunk_at(0U),semantic_role::spine1);
    assign(2U,trunk_at(1U),semantic_role::spine2);
    assign(3U,trunk_at(2U),semantic_role::chest);
    assign(4U,head.front(),semantic_role::neck);
    assign(6U,head.back(),semantic_role::head);
    const std::array<semantic_role,4> left_arm_roles{{semantic_role::left_shoulder,semantic_role::left_upper_arm,
        semantic_role::left_forearm,semantic_role::left_hand}};
    const std::array<semantic_role,4> right_arm_roles{{semantic_role::right_shoulder,semantic_role::right_upper_arm,
        semantic_role::right_forearm,semantic_role::right_hand}};
    const std::array<semantic_role,4> left_leg_roles{{semantic_role::left_upper_leg,semantic_role::left_shin,
        semantic_role::left_foot,semantic_role::left_toe}};
    const std::array<semantic_role,4> right_leg_roles{{semantic_role::right_upper_leg,semantic_role::right_shin,
        semantic_role::right_foot,semantic_role::right_toe}};
    for (std::size_t index=0;index<4U;++index) {
        assign(10U+index,arm_candidates[0][index],left_arm_roles[index]);
        assign(16U+index,arm_candidates[1][index],right_arm_roles[index]);
        assign(22U+index,legs[0][index],left_leg_roles[index]);
        assign(26U+index,legs[1][index],right_leg_roles[index]);
    }

    const vec3 root_position=generated.rest_positions[root];
    const float stature=std::max(distance(root_position,generated.rest_positions[head.back()]),1.0e-6F);
    float score=1.0F;
    if (generated.rest_positions[head.back()].y<=root_position.y+.2F*stature) score-=.35F;
    if (generated.rest_positions[legs[0].back()].y>=root_position.y-.15F*stature ||
        generated.rest_positions[legs[1].back()].y>=root_position.y-.15F*stature) score-=.25F;
    if (generated.rest_positions[arm_candidates[0].back()].x<=root_position.x+.15F*stature ||
        generated.rest_positions[arm_candidates[1].back()].x>=root_position.x-.15F*stature) score-=.25F;
    score=std::clamp(score,0.0F,1.0F);
    output.report.core_score=score;
    output.report.confidence=score;
    output.report.alternative_margin=.5F*score;
    output.report.mapped_core_joints=22U;
    output.report.ignored_terminal_joints=
        subtree_size(subtree_size,arm_candidates[0].back())-1U+
        subtree_size(subtree_size,arm_candidates[1].back())-1U;
    if (!options.allow_flexible_hands && output.report.ignored_terminal_joints!=30U)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated hand topology differs from the fixed 52-joint humanoid"));
    if (score<options.minimum_confidence)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated humanoid confidence "+std::to_string(score)+
            " is below the requested threshold "+std::to_string(options.minimum_confidence)));
    return output;
}

result<motion> retarget_soma30_motion_to_generated(
    const motion & animation,const skeleton & generated,const humanoid_mapping & mapping,
    const retarget_options & options) {
    auto valid_source=validate(animation);if (!valid_source) return tl::unexpected(valid_source.error());
    auto valid_target=validate(generated);if (!valid_target) return tl::unexpected(valid_target.error());
    if (!std::isfinite(options.minimum_confidence) || options.minimum_confidence<0.0F ||
        options.minimum_confidence>1.0F || mapping.report.confidence<options.minimum_confidence)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated humanoid mapping does not meet the requested confidence"));
    if (mapping.generated_roles.size()!=generated.names.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated humanoid mapping belongs to a different skeleton"));

    std::array<std::size_t,30> source{};
    for (std::size_t index=0;index<soma_names.size();++index) {
        const auto joint=find(animation.rig,soma_names[index]);
        if (!joint) return tl::unexpected(detail::fail(error_code::invalid_argument,
            "SOMA30 skeleton is missing joint "+std::string{soma_names[index]}));
        source[index]=*joint;
    }
    std::vector<std::int32_t> target_source(generated.names.size(),-1);
    for (std::size_t soma=0;soma<mapping.soma_to_generated.size();++soma) {
        const auto target=mapping.soma_to_generated[soma];
        if (target<0) continue;
        if (static_cast<std::size_t>(target)>=target_source.size() || target_source[static_cast<std::size_t>(target)]>=0)
            return tl::unexpected(detail::fail(error_code::invalid_argument,
                "generated humanoid mapping contains an invalid or duplicate target"));
        target_source[static_cast<std::size_t>(target)]=static_cast<std::int32_t>(source[soma]);
    }
    motion output;output.frames=animation.frames;output.frames_per_second=animation.frames_per_second;output.rig=generated;
    output.local_rotations.assign(output.frames*generated.names.size(),{});
    // Both Kimodo and SkinTokens GLBs express local rotations in the same
    // glTF coordinate frame and encode the rest shape in node translations.
    // Transfer local tracks through the semantic hierarchy. Independently
    // aligning every joint's global rotation to its bone direction creates a
    // different basis at parent and child, which manufactures rotations in a
    // child whose source-local track is identity and compounds down the limb.
    std::vector<std::vector<std::size_t>> source_paths(generated.names.size());
    for (std::size_t target=0;target<generated.names.size();++target) {
        if (target_source[target]<0) continue;
        std::int32_t target_parent=generated.parents[target];
        while (target_parent>=0 && target_source[static_cast<std::size_t>(target_parent)]<0)
            target_parent=generated.parents[static_cast<std::size_t>(target_parent)];
        const std::int32_t source_parent=target_parent<0 ? -1 :
            target_source[static_cast<std::size_t>(target_parent)];
        std::int32_t cursor=target_source[target];
        while (cursor!=source_parent && cursor>=0) {
            source_paths[target].push_back(static_cast<std::size_t>(cursor));
            cursor=animation.rig.parents[static_cast<std::size_t>(cursor)];
        }
        if (cursor!=source_parent)
            return tl::unexpected(detail::fail(error_code::invalid_argument,
                "semantic target hierarchy does not follow the SOMA30 hierarchy"));
        std::reverse(source_paths[target].begin(),source_paths[target].end());
    }
    for (std::size_t frame=0;frame<output.frames;++frame) {
        for (std::size_t target=0;target<generated.names.size();++target) {
            quat local{};
            for (const auto source_joint:source_paths[target])
                local=qnorm(qmul(local,animation.local_rotations[
                    frame*animation.rig.names.size()+source_joint]));
            output.local_rotations[frame*generated.names.size()+target]=local;
        }
        if (options.fingers==finger_transfer::map_soma_endpoints) {
            for (const auto pair:std::array<std::pair<std::size_t,std::array<std::size_t,2>>,2>{{
                {13U,{14U,15U}},{19U,{20U,21U}}}}) {
                const auto hand=mapping.soma_to_generated[pair.first];
                if (hand<0) continue;
                std::size_t branch=0U;
                for (std::size_t target=0;target<generated.names.size();++target) {
                    if (generated.parents[target]!=hand) continue;
                    const auto endpoint=source[pair.second[std::min(branch,std::size_t{1U})]];
                    output.local_rotations[frame*generated.names.size()+target]=
                        animation.local_rotations[frame*animation.rig.names.size()+endpoint];
                    ++branch;
                }
            }
        }
    }

    const auto source_head=source[6],source_left_toe=source[25],source_right_toe=source[29];
    const auto target_index=[&](std::size_t soma) { return static_cast<std::size_t>(mapping.soma_to_generated[soma]); };
    const vec3 source_feet=scale(add(animation.rig.rest_positions[source_left_toe],animation.rig.rest_positions[source_right_toe]),.5F);
    const vec3 target_feet=scale(add(generated.rest_positions[target_index(25U)],generated.rest_positions[target_index(29U)]),.5F);
    const float source_height=distance(animation.rig.rest_positions[source_head],source_feet);
    const float target_height=distance(generated.rest_positions[target_index(6U)],target_feet);
    if (source_height<=1.0e-8F || target_height<=1.0e-8F)
        return tl::unexpected(detail::fail(error_code::invalid_argument,"cannot scale root motion for a degenerate humanoid"));
    const float travel=options.scale_root_motion?target_height/source_height:1.0F;
    const vec3 first=animation.root_translations.front(),root=generated.rest_positions[target_index(0U)];
    output.root_translations.resize(output.frames);
    for (std::size_t frame=0;frame<output.frames;++frame)
        output.root_translations[frame]=add(root,scale(
            sub(animation.root_translations[frame],first),travel));
    return output;
}

result<motion_divergence_report> compare_soma30_to_generated(
    const motion & source_motion,const motion & target_motion,
    const humanoid_mapping & mapping) {
    auto valid_source=validate(source_motion);if (!valid_source) return tl::unexpected(valid_source.error());
    auto valid_target=validate(target_motion);if (!valid_target) return tl::unexpected(valid_target.error());
    if (source_motion.frames!=target_motion.frames)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "source and generated animations have different frame counts"));
    if (mapping.generated_roles.size()!=target_motion.rig.names.size())
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "generated humanoid mapping belongs to a different skeleton"));
    std::array<std::size_t,30> source{};
    for (std::size_t soma=0;soma<soma_names.size();++soma) {
        const auto joint=find(source_motion.rig,soma_names[soma]);
        if (!joint) return tl::unexpected(detail::fail(error_code::invalid_argument,
            "SOMA30 skeleton is missing joint "+std::string{soma_names[soma]}));
        source[soma]=*joint;
    }
    const auto target_index=[&](std::size_t soma) -> result<std::size_t> {
        const auto target=mapping.soma_to_generated[soma];
        if (target<0 || static_cast<std::size_t>(target)>=target_motion.rig.names.size())
            return tl::unexpected(detail::fail(error_code::invalid_argument,
                "generated mapping is missing a required scale joint"));
        return static_cast<std::size_t>(target);
    };
    auto target_head=target_index(6U),target_left_toe=target_index(25U),target_right_toe=target_index(29U);
    if (!target_head) return tl::unexpected(target_head.error());
    if (!target_left_toe) return tl::unexpected(target_left_toe.error());
    if (!target_right_toe) return tl::unexpected(target_right_toe.error());
    const vec3 source_feet=scale(add(source_motion.rig.rest_positions[source[25]],
        source_motion.rig.rest_positions[source[29]]),.5F);
    const vec3 target_feet=scale(add(target_motion.rig.rest_positions[*target_left_toe],
        target_motion.rig.rest_positions[*target_right_toe]),.5F);
    const float source_scale=distance(source_motion.rig.rest_positions[source[6]],source_feet);
    const float target_scale=distance(target_motion.rig.rest_positions[*target_head],target_feet);
    if (source_scale<=1.0e-8F || target_scale<=1.0e-8F)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "cannot compare degenerate humanoid skeletons"));

    motion_divergence_report output;
    double sum=0.0;std::size_t samples=0U;
    const vec3 source_rest_root=source_motion.rig.rest_positions[source[0]];
    const auto target_root_result=target_index(0U);
    if (!target_root_result) return tl::unexpected(target_root_result.error());
    const auto target_root=*target_root_result;
    const vec3 target_rest_root=target_motion.rig.rest_positions[target_root];
    for (std::size_t frame=0;frame<source_motion.frames;++frame) {
        const auto source_pose=posed_joints(source_motion,frame);
        const auto target_pose=posed_joints(target_motion,frame);
        for (std::size_t soma=0;soma<mapping.soma_to_generated.size();++soma) {
            const auto target=mapping.soma_to_generated[soma];
            if (target<0) continue;
            vec3 source_delta{},target_delta{};
            if (soma==0U) {
                source_delta=scale(sub(source_motion.root_translations[frame],
                    source_motion.root_translations.front()),1.0F/source_scale);
                target_delta=scale(sub(target_motion.root_translations[frame],
                    target_motion.root_translations.front()),1.0F/target_scale);
            } else {
                source_delta=sub(scale(sub(source_pose[source[soma]],source_pose[source[0]]),1.0F/source_scale),
                    scale(sub(source_motion.rig.rest_positions[source[soma]],source_rest_root),1.0F/source_scale));
                const auto target_joint=static_cast<std::size_t>(target);
                target_delta=sub(scale(sub(target_pose[target_joint],target_pose[target_root]),1.0F/target_scale),
                    scale(sub(target_motion.rig.rest_positions[target_joint],target_rest_root),1.0F/target_scale));
            }
            const float error=distance(source_delta,target_delta);
            sum+=error;++samples;
            output.per_joint_maximum[soma]=std::max(output.per_joint_maximum[soma],error);
            if (error>output.maximum_normalized_displacement_error) {
                output.maximum_normalized_displacement_error=error;
                output.worst_frame=frame;
                output.worst_soma30_joint=static_cast<std::uint32_t>(soma);
            }
        }
    }
    output.mean_normalized_displacement_error=samples?
        static_cast<float>(sum/static_cast<double>(samples)):0.0F;
    return output;
}

result<humanoid_match_report> retarget_soma30_glb_file(
    const std::filesystem::path & generated_rigged_glb,
    const std::filesystem::path & soma30_motion_glb,
    const std::filesystem::path & output_glb,
    const humanoid_match_options & match_options,
    const retarget_options & options) {
    auto asset=load_skinned_glb_file(generated_rigged_glb);
    if (!asset) return tl::unexpected(asset.error());
    auto source=load_kimodo_glb_file(soma30_motion_glb);
    if (!source) return tl::unexpected(source.error());
    if (identify_rig(source->rig)!=rig_kind::soma30)
        return tl::unexpected(detail::fail(error_code::invalid_argument,
            "motion GLB is not a recognized SOMA30 animation"));
    auto mapping=match_generated_humanoid(asset->binding.rig,match_options);
    if (!mapping) return tl::unexpected(mapping.error());
    auto animation=retarget_soma30_motion_to_generated(
        *source,asset->binding.rig,*mapping,options);
    if (!animation) return tl::unexpected(animation.error());
    auto saved=save_skinned_animation_glb_file(
        output_glb,asset->geometry,asset->binding,*animation);
    if (!saved) return tl::unexpected(saved.error());
    return mapping->report;
}

result<motion> retarget_motion_to_rig(const motion & animation,const skeleton & target) {
    auto valid_source=validate(animation);if (!valid_source) return tl::unexpected(valid_source.error());
    auto valid_target=validate(target);if (!valid_target) return tl::unexpected(valid_target.error());
    const auto source_bounds=center_scale(animation.rig),target_bounds=center_scale(target);
    if (source_bounds.second<=1.0e-8F || target_bounds.second<=1.0e-8F)
        return tl::unexpected(detail::fail(error_code::invalid_argument,"cannot retarget a degenerate skeleton"));
    std::vector<vec3> source_positions,target_positions;
    for (const auto value:animation.rig.rest_positions) source_positions.push_back(scale(sub(value,source_bounds.first),1.0F/source_bounds.second));
    for (const auto value:target.rest_positions) target_positions.push_back(scale(sub(value,target_bounds.first),1.0F/target_bounds.second));
    const auto descendant=[&](std::size_t candidate,std::int32_t ancestor) {
        if (ancestor<0) return true;
        std::int32_t cursor=static_cast<std::int32_t>(candidate);
        while (cursor>=0) { if (cursor==ancestor) return true;cursor=animation.rig.parents[static_cast<std::size_t>(cursor)]; }
        return false;
    };
    std::vector<std::size_t> mapping(target.names.size(),0U);
    for (std::size_t joint=1U;joint<target.names.size();++joint) {
        const auto mapped_parent=mapping[static_cast<std::size_t>(target.parents[joint])];
        float best=std::numeric_limits<float>::infinity();
        for (std::size_t candidate=0;candidate<source_positions.size();++candidate) {
            if (!descendant(candidate,static_cast<std::int32_t>(mapped_parent))) continue;
            float score=distance(source_positions[candidate],target_positions[joint]);score*=score;
            if ((source_positions[candidate].x<-.03F)!=(target_positions[joint].x<-.03F) && std::abs(target_positions[joint].x)>.08F) score+=1.0F;
            if (score<best) { best=score;mapping[joint]=candidate; }
        }
    }
    motion output;
    output.frames=animation.frames;output.frames_per_second=animation.frames_per_second;output.rig=target;
    output.local_rotations.resize(output.frames*target.names.size());
    for (std::size_t frame=0;frame<output.frames;++frame) for (std::size_t joint=0;joint<target.names.size();++joint)
        output.local_rotations[frame*target.names.size()+joint]=animation.local_rotations[frame*animation.rig.names.size()+mapping[joint]];
    output.root_translations.resize(output.frames);
    const auto first=animation.root_translations.front(),root=target.rest_positions.front();
    const float travel=target_bounds.second/source_bounds.second;
    for (std::size_t frame=0;frame<output.frames;++frame) {
        const auto value=animation.root_translations[frame];
        output.root_translations[frame]={root.x+(value.x-first.x)*travel,root.y+(value.y-first.y)*travel,root.z+(value.z-first.z)*travel};
    }
    return output;
}

} // namespace skintokens
